#pragma once

#include "psl/serialization/parser.hpp"
#include <algorithm>
#include <stdexcept>

namespace psl::serialization::format {
constexpr auto parse_identifier(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<accumulate(
	  text_parser<std::not_equal_to>("#"sv),
	  accumulate_many(none_of(" \n\r\t:"), 1),
	  [](auto sv1, auto sv2) {
		  // todo this isn't exactly safe, find better way
		  return psl::string8::view {sv2.data() - sv1.size(), sv1.size() + sv2.size()};
	  })>
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
	constexpr auto parser = skip_whitespace_parser()<accumulate_many(none_of(" \n\r\t<[="sv))> skip_whitespace_parser();
	const auto result	  = parser(view);

	// we do a look ahead to figure out if the next statement would be either a template, or an attribute
	constexpr auto next_symbol_parser = any_of("<["sv);
	if(const auto next = next_symbol_parser(result.view()); next) {
		throw std::runtime_error("not implemented");
	}
	return result;
}
constexpr auto parse_type_assignment_seperator(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<text_parser("="sv)> skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_assignment_begin(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;

	constexpr auto parser = skip_whitespace_parser()<text_parser("{"sv)> skip_whitespace_parser();
	return parser(view);
}

constexpr auto parse_assigment_value(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	parse_result_t<psl::string8::view> result {invalid_result};
	// skip all space, then check if we are in a literal (either ' or " ), then check if it's a strong literal
	// we parse the value based on that.
	view = skip_whitespace_parser()(view).view();


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
			  }) > text_parser(view.substr(0, 3));
			result = get_value(is_strong_literal.view());
		} else {
			const auto get_value = accumulate_many([c = view.at(0)](parse_view_t view) -> parse_result_t<char> {
									   if(view.size() >= 1 && view.at(0) == c) {
										   return invalid_result;
									   }
									   return {view.substr(1), view.at(0)};
								   }) > text_parser(view.substr(0, 1));
			result				 = get_value(is_literal.view());
		}
	} else {
		constexpr auto get_value = accumulate_many(none_of(" \n\r\t,}"));
		result					 = get_value(view);
	}
	if(!result) {
		return invalid_result;
	}

	view = result.view();

	// next we check if we have a comma seperator, in case of array.
	if(const auto seperator = (skip_whitespace_parser() < text_parser(","sv))(view); seperator) {
		throw std::runtime_error("not implemented");
	}

	return result;
}

constexpr auto parse_assignment_end(psl::string8::view view)
  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
	using namespace psl::serialization::parser;
	constexpr auto parser = skip_whitespace_parser()<text_parser("}"sv) < skip_whitespace_parser() < text_parser(";"sv)>
	skip_whitespace_parser();
	return parser(view);
}

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

	if(const auto seperator = parse_type_assignment_seperator(view); !seperator) {
		return invalid_result;
	} else {
		view = seperator.view();
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
