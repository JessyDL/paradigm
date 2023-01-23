#pragma once

#include "psl/serialization/parser.hpp"
#include <algorithm>
#include <stdexcept>
#include <strtype/strtype.hpp>

namespace psl::serialization::format {

template <typename T>
struct type_map {
	static constexpr psl::string8::view value = {strtype::stringify_typename<T>()};
};

template <typename T>
static constexpr psl::string8::view type_map_v = type_map<T>::value;

enum class field_type_t { object, value };

constexpr auto parse_identifier(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<accumulate(
	  [](auto sv1, auto sv2) {
		  // todo this isn't exactly safe, find better way
		  return psl::string8::view {sv2.data() - sv1.size(), sv1.size() + sv2.size()};
	  },
	  text_parser<std::not_equal_to>("#"sv),
	  accumulate_many(none_of(" \n\r\t:")))>
	skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_identifier_type_seperator(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<text_parser(":"sv)> skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_type(psl::string8::view view) -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;

	// accumulate all except anything template related, attribute related, or assigment (in case of no space)
	constexpr auto parser = skip_whitespace_parser() < accumulate_many(none_of("[{;"sv));
	auto result			  = parser(view);
	if(!result) {
		return invalid_result;
	}
	constexpr psl::string8::view space {" \n\r\t"};

	// todo: this block should actually calculate all non-space characters, and inject that into a buffer
	size_t actual_size {0};
	for(auto it = result.value().rbegin(); it != result.value().crend(); ++it) {
		if(std::none_of(space.begin(), space.end(), [c = *it](auto val) { return c == val; })) {
			actual_size = std::distance(it, result.value().crend());
			break;
		}
	}
	result = {result.view(), result.value().substr(0, actual_size)};

	// we do a look ahead to figure out if the next statement is an attribute
	/*if(!result.view().empty() && result.view().at(0) == '[') {
		throw std::runtime_error("not implemented");
	}*/
	return result;
}

constexpr auto parse_attribute_begin(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<text_parser("["sv)> skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_attribute_identfier(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<accumulate_many(none_of("=]{"sv))> skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_attribute_assignment_begin(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<text_parser("{"sv)> skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_assigment_value(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view>;

constexpr auto parse_attribute_assignment_value(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	return parse_assigment_value(view);
}

constexpr auto parse_attribute_assignment_end(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<text_parser("}"sv)> skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_attribute_end(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<text_parser("]"sv)> skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_assignment_begin(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;

	constexpr auto parser = skip_whitespace_parser()<text_parser("{"sv)> skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_assigment_value_more(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<std::monostate> {
	using namespace psl::serialization::parser;
	constexpr auto parser =
	  fmap([](auto) -> std::monostate { return {}; }, skip_whitespace_parser() < text_parser(","));
	return parser(view);
}

constexpr auto parse_assigment_value(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	parse_result_t<psl::string8::view> result {invalid_result};
	// skip all space, then check if we are in a literal (either ' or " ), then check if it's a strong literal
	// we parse the value based on that.
	view = skip_whitespace_parser()(view).view();

	if(!view.empty() && view.at(0) == '}') {
		return {view, psl::string8::view {}};
	}

	if(const auto is_literal = any_of("\"'"sv)(view); is_literal) {
		if(const auto is_strong_literal =
			 exactly(2, char_parser(is_literal.value()), std::monostate {}, [](auto lhs, auto) { return lhs; })(
			   is_literal.view());
		   is_strong_literal) {
			const auto get_value =
			  accumulate_many([c = view.at(0)](parse_view_t view) -> parse_result_t<char> {
				  if(view.size() >= 3 &&
					 std::all_of(view.begin(), std::next(view.begin(), 3), [c](char val) { return c == val; })) {
					  return invalid_result;
				  }
				  return {view.substr(1), view.at(0)};
			  }) > text_parser(view.substr(0, 3)) > skip_whitespace_parser();
			result = get_value(is_strong_literal.view());
		} else {
			const auto get_value = accumulate_many([c = view.at(0)](parse_view_t view) -> parse_result_t<char> {
									   if(view.size() >= 1 && view.at(0) == c) {
										   return invalid_result;
									   }
									   return {view.substr(1), view.at(0)};
								   }) > text_parser(view.substr(0, 1)) > skip_whitespace_parser();
			result				 = get_value(is_literal.view());
		}
	} else {
		constexpr auto get_value = accumulate_many(none_of(" \n\r\t,}"), 1) > skip_whitespace_parser();
		result					 = get_value(view);
	}
	if(!result) {
		return invalid_result;
	}

	view = result.view();

	// next we check if we have a comma seperator, in case of array.
	/*if(const auto seperator = (skip_whitespace_parser() < text_parser(","sv))(view); seperator) {
		throw std::runtime_error("not implemented");
	}*/

	return result;
}

constexpr size_t count_assignments(psl::string8::view view) {
	size_t result {0};
	while(true) {
		if(const auto parse = parse_assigment_value(view); !parse) {
			return result;
		} else if(const auto next = parse_assigment_value_more(parse.view()); next) {
		} else {
			view = parse.view();
			result += 1;
		}
	}
}

constexpr auto parse_assignment_end(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<text_parser("}"sv) < skip_whitespace_parser() < text_parser(";"sv)>
	skip_whitespace_parser();
	return parser(view);
}


namespace size {
	constexpr auto parse_identifier(psl::string8::view view) -> parser::parse_result_t<size_t> {
		if(const auto val = format::parse_identifier(view); val) {
			return {val.view(), val.value().size()};
		}
		return parser::invalid_result;
	}

	constexpr auto parse_identifier_type_seperator(psl::string8::view view) -> parser::parse_result_t<size_t> {
		if(const auto val = format::parse_identifier_type_seperator(view); val) {
			return {val.view(), 0};
		}
		return parser::invalid_result;
	}

	constexpr auto parse_type(psl::string8::view view) -> parser::parse_result_t<std::pair<size_t, field_type_t>> {
		if(const auto val = format::parse_type(view); val) {
			field_type_t field = {field_type_t ::value};
			if(val.value() == "object"sv) {
				field = field_type_t::object;
			}
			return {val.view(), std::pair<size_t, field_type_t> {val.value().size(), field}};
		}
		return parser::invalid_result;
	}


	constexpr auto parse_attribute_begin(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<size_t> {
		if(const auto val = format::parse_attribute_begin(view); val) {
			return {val.view(), 0};
		}
		return parser::invalid_result;
	}

	constexpr auto parse_attribute_identfier(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<size_t> {
		if(const auto val = format::parse_attribute_identfier(view); val) {
			return {val.view(), val.value().size()};
		}
		return parser::invalid_result;
	}


	constexpr auto parse_attribute_assignment_begin(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<size_t> {
		if(const auto val = format::parse_attribute_assignment_begin(view); val) {
			return {val.view(), 0};
		}
		return parser::invalid_result;
	}

	constexpr auto parse_assigment_value(psl::string8::view view) -> parser::parse_result_t<size_t>;
	constexpr auto parse_attribute_assignment_value(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<size_t> {
		return size::parse_assigment_value(view);
	}

	constexpr auto parse_attribute_assignment_end(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<size_t> {
		if(const auto val = format::parse_attribute_assignment_end(view); val) {
			return {val.view(), 0};
		}
		return parser::invalid_result;
	}

	constexpr auto parse_attribute_end(psl::string8::view view) -> psl::serialization::parser::parse_result_t<size_t> {
		if(const auto val = format::parse_attribute_end(view); val) {
			return {val.view(), 0};
		}
		return parser::invalid_result;
	}


	constexpr auto parse_assignment_begin(psl::string8::view view) -> parser::parse_result_t<size_t> {
		if(const auto val = format::parse_assignment_begin(view); val) {
			return {val.view(), 0};
		}
		return parser::invalid_result;
	}

	constexpr auto parse_assigment_value(psl::string8::view view) -> parser::parse_result_t<size_t> {
		if(const auto val = format::parse_assigment_value(view); val) {
			return {val.view(), val.value().size()};
		}
		return parser::invalid_result;
	}

	constexpr auto parse_assigment_value_more(psl::string8::view view) -> parser::parse_result_t<size_t> {
		if(const auto val = format::parse_assigment_value_more(view); val) {
			return {val.view(), 0};
		}
		return parser::invalid_result;
	}

	constexpr auto parse_assignment_end(psl::string8::view view) -> parser::parse_result_t<size_t> {
		if(const auto val = format::parse_assignment_end(view); val) {
			return {val.view(), val.value().size()};
		}
		return parser::invalid_result;
	}

	constexpr auto parse_field(psl::string8::view view) -> parser::parse_result_t<size_t> {
		constexpr auto counter = []<typename T>(T&& parser, size_t atleast = 0) {
			return parser::many(std::forward<T>(parser), size_t {0}, std::plus {}, atleast);
		};

		field_type_t type {field_type_t ::value};

		constexpr auto parser = accumulate(
		  std::plus {},
		  size::parse_identifier,
		  size::parse_identifier_type_seperator,
		  parser::fmap(
			[&type](const std::pair<size_t, field_type_t>& pair) -> size_t {
				type = pair.second;
				return pair.first;
			},
			size::parse_type),
		  counter(accumulate(std::plus {},
							 size::parse_attribute_begin,
							 size::parse_attribute_identfier,
							 accumulate(std::plus {},
										size::parse_attribute_assignment_begin,
										size::parse_attribute_assignment_value,
										size::parse_attribute_assignment_end,
										size::parse_attribute_end) |
							   size::parse_attribute_end)),
		  size::parse_assignment_begin,
		  [&type](parser::parse_view_t view) -> parser::parse_result_t<size_t> {
			  constexpr auto counter = []<typename T>(T&& parser, size_t atleast = 0) {
				  return parser::many(std::forward<T>(parser), size_t {0}, std::plus {}, atleast);
			  };
			  switch(type) {
			  case field_type_t::object:
				  return counter(size::parse_field)(view);
			  case field_type_t::value:
				  return counter(accumulate(std::plus {},
											counter([](parser::parse_view_t view) -> parser::parse_result_t<size_t> {
												const auto res = size::parse_assigment_value(view);
												if(res) {
													const auto sep = size::parse_assigment_value_more(res.view());
													if(res.value() == 0 && !sep) {
														return parser::invalid_result;
													} else if(sep) {
														return {sep.view(), res.value()};
													}
												}
												return res;
											}),
											size::parse_assignment_end))(view);
			  }
			  return parser::invalid_result;
		  });
		return parser(view);
	}

	constexpr auto parse(psl::string8::view view) -> parser::parse_result_t<size_t> {
		constexpr auto counter = []<typename T>(T&& parser, size_t atleast = 0) {
			return parser::many(std::forward<T>(parser), size_t {0}, std::plus {}, atleast);
		};
		constexpr auto parser = counter(size::parse_field);

		return parser(view);
	}
}	 // namespace size

struct type_t {
	psl::string8::view name;
};
struct value_t {
	psl::string8::view value;
};

struct identifier_t {
	psl::string8::view name;
	type_t type;
	value_t value;
};


constexpr auto parse_field(psl::string8::view view) -> psl::serialization::parser::parse_result_t<identifier_t> {
	using namespace psl::serialization::parser;
	identifier_t result {};
	if(const auto identifier = parse_identifier(view); !identifier) {
		return invalid_result;
	} else {
		result.name = identifier.value();
		view		= identifier.view();
	}

	if(const auto seperator = parse_identifier_type_seperator(view); !seperator) {
		return invalid_result;
	} else {
		view = seperator.view();
	}

	if(const auto type = parse_type(view); !type) {
		return invalid_result;
	} else {
		view			 = type.view();
		result.type.name = type.value();
	}

	if(const auto seperator = parse_assignment_begin(view); !seperator) {
		return invalid_result;
	} else {
		view = seperator.view();
	}

	if(const auto value = parse_assigment_value(view); !value) {
		return invalid_result;
	} else {
		view			   = value.view();
		result.value.value = value.value();
	}

	if(const auto seperator = parse_assignment_end(view); !seperator) {
		return invalid_result;
	} else {
		view = seperator.view();
	}

	return {view, result};
}
}	 // namespace psl::serialization::format
