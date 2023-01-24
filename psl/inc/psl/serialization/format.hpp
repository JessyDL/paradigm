#pragma once

#include "psl/collections/compile_time_string.hpp"
#include "psl/serialization/parser.hpp"
#include <algorithm>
#include <stdexcept>
#include <strtype/strtype.hpp>

namespace psl::serialization::format {
inline namespace _details {
	/// \brief Details how to parse the given field, what rules the parser shoud follow
	enum class value_parse_type { object, value };

	struct format_storage_info_t {
		friend constexpr format_storage_info_t operator+(format_storage_info_t const& lhs,
														 format_storage_info_t const& rhs) noexcept {
			return format_storage_info_t {.identifiers		 = lhs.identifiers + rhs.identifiers,
										  .types			 = lhs.types + rhs.types,
										  .attributes		 = lhs.attributes + rhs.attributes,
										  .attributes_values = lhs.attributes_values + rhs.attributes_values,
										  .values			 = lhs.values + rhs.values,
										  .storage_size		 = lhs.storage_size + rhs.storage_size};
		}

		size_t identifiers {0};
		size_t types {0};
		size_t attributes {0};
		size_t attributes_values {0};
		size_t values {0};
		size_t storage_size {0};
	};

	constexpr auto decode_type(psl::string8::view view) -> value_parse_type {
		if(view == "object"sv) {
			return value_parse_type::object;
		} else if(view.starts_with("object"sv)) {
			auto offset = view.find_first_not_of(" \n\r\t"sv, 6);
			if(offset != view.npos && view[offset] == '<') {
				return value_parse_type::object;
			}
		}
		return value_parse_type::value;
	}

	constexpr auto parse_identifier(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
		using namespace psl::serialization::parser;
		constexpr auto parser = skip_whitespace_parser()<accumulate(
		  [](auto sv1, auto sv2) {
			  // todo this isn't exactly safe, find better way
			  return psl::string8::view {sv2.data() - sv1.size(), sv1.size() + sv2.size()};
		  },
		  parser::none_of<psl::string8::view>("}#"),
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

	constexpr auto parse_type(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
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
			auto literal_parser = [c = is_literal.value()](size_t count) {
				return exactly(count, char_parser(c), psl::string8::view {}, [](auto lhs, auto) { return lhs; });
			};

			if(const auto is_strong_literal = literal_parser(2)(is_literal.view()); is_strong_literal) {
				const auto get_value =
				  accumulate_many([c = view.at(0)](parse_view_t view) -> parse_result_t<char> {
					  if(view.size() >= 3 &&
						 std::all_of(view.begin(), std::next(view.begin(), 3), [c](char val) { return c == val; })) {
						  return invalid_result;
					  }
					  return {view.substr(1), view.at(0)};
				  }) > literal_parser(3) > skip_whitespace_parser();
				result = get_value(is_strong_literal.view());
			} else {
				const auto get_value = accumulate_many([c = view.at(0)](parse_view_t view) -> parse_result_t<char> {
										   if(view.size() >= 1 && view.at(0) == c) {
											   return invalid_result;
										   }
										   return {view.substr(1), view.at(0)};
									   }) > literal_parser(1) > skip_whitespace_parser();
				result				 = get_value(is_literal.view());
			}
		} else {
			constexpr auto get_value = accumulate_many(none_of(" \n\r\t,}"), 1) > skip_whitespace_parser();
			result					 = get_value(view);
		}

		return result;
	}

	constexpr auto parse_assignment_end(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
		using namespace psl::serialization::parser;
		constexpr auto parser =
		  skip_whitespace_parser()<text_parser("}"sv) < skip_whitespace_parser() < text_parser(";"sv)>
		  skip_whitespace_parser();
		return parser(view);
	}

	struct size_calculator_t {
		using return_type = format_storage_info_t;
		static constexpr auto transform_identifier(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.identifiers  = 1;
			info.storage_size = view.size();
			return info;
		}

		static constexpr auto transform_type(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.types		  = 1;
			info.storage_size = view.size();
			return info;
		}

		static constexpr auto transform_attribute(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.attributes	  = 1;
			info.storage_size = view.size();
			return info;
		}

		static constexpr auto transform_attribute_value(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.attributes_values = 1;
			info.storage_size	   = view.size();
			return info;
		}

		static constexpr auto transform_value(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.values		  = 1;
			info.storage_size = view.size();
			return info;
		}

		template <parser::IsParser T>
		static constexpr auto wrap(T&& parser) {
			return
			  [parser = std::forward<T>(parser)](parser::parse_view_t view) -> parser::parse_result_t<return_type> {
				  const auto result = parser(view);
				  if(result) {
					  return {result.view(), return_type {}};
				  }
				  return parser::invalid_result;
			  };
		}
		template <parser::IsParser T, typename Fn>
		static constexpr auto wrap(T&& parser, Fn fn) {
			return [parser = std::forward<T>(parser),
					fn	   = std::forward<Fn>(fn)](parser::parse_view_t view) -> parser::parse_result_t<return_type> {
				const auto result = parser(view);
				if(result) {
					return {result.view(), fn(result.value())};
				}
				return parser::invalid_result;
			};
		}
	};

	template <typename T>
	constexpr auto parse(psl::string8::view text) -> parser::parse_result_t<typename T::return_type> {
		using return_type = typename T::return_type;

		// parse until the type seperator
		constexpr auto parse_identifier =
		  parser::accumulate(std::plus {},
							 T::wrap(_details::parse_identifier, T::transform_identifier),
							 T::wrap(_details::parse_identifier_type_seperator));

		// parses the type, and decides to run either the value parser or the object parser dependent on the outcome
		constexpr auto parse_type = []<parser::IsParser ValueFn, parser::IsParser ObjectFn>(ValueFn&& value_parser,
																							ObjectFn&& object_parser) {
			return [value_parser  = std::forward<ValueFn>(value_parser),
					object_parser = std::forward<ObjectFn>(object_parser)](
					 parser::parse_view_t view) -> parser::parse_result_t<return_type> {
				const auto result = _details::parse_type(view);
				if(!result) {
					return parser::invalid_result;
				}
				auto result_info = T::transform_type(result.value());

				const auto type = _details::decode_type(result.value());
				switch(type) {
				case value_parse_type::value: {
					const auto value = value_parser(result.view());
					if(!value) {
						return parser::invalid_result;
					}
					return {value.view(), value.value() + result_info};
				}
				case value_parse_type::object: {
					const auto value = object_parser(result.view());
					if(!value) {
						return parser::invalid_result;
					}
					return {value.view(), value.value() + result_info};
				}
				}
				return parser::invalid_result;
			};
		};

		// parse all attributes until no more can be found
		constexpr auto parse_attributes =
		  parser::many(parser::accumulate(std::plus {},
										  T::wrap(_details::parse_attribute_begin),
										  T::wrap(_details::parse_attribute_identfier, T::transform_attribute),
										  parser::accumulate(std::plus {},
															 T::wrap(_details::parse_attribute_assignment_begin),
															 T::wrap(_details::parse_attribute_assignment_value,
																	 T::transform_attribute_value),
															 T::wrap(_details::parse_attribute_assignment_end),
															 T::wrap(_details::parse_attribute_end)) |
											T::wrap(_details::parse_attribute_end)),
					   return_type {},
					   std::plus {});

		// parses all values until the end statement is found
		constexpr auto parse_assignment_values = parser::accumulate(
		  std::plus {},
		  T::wrap(_details::parse_assignment_begin),
		  T::wrap(_details::parse_assigment_value, T::transform_value),
		  parser::many(parser::accumulate(std::plus {},
										  T::wrap(_details::parse_assigment_value_more) <
											T::wrap(_details::parse_assigment_value, T::transform_value)),
					   return_type {},
					   std::plus {}),
		  T::wrap(_details::parse_assignment_end));

		// parses all objects until the end statement is found, we internally first scan for the end statement to decide
		// if a next object even exists
		constexpr auto parse_assignment_objects =
		  parser::accumulate(std::plus {},
							 T::wrap(_details::parse_assignment_begin),
							 parser::many(
							   [](parser::parse_view_t view) -> parser::parse_result_t<return_type> {
								   if(const auto end = _details::parse_assignment_end(view); end) {
									   return parser::invalid_result;
								   }
								   return parse<T>(view);
							   },
							   return_type {},
							   std::plus {}),
							 T::wrap(_details::parse_assignment_end));

		// the combination of all previous parsers into a continuous statement that recursively scans the fields
		constexpr auto parser =
		  parser::accumulate(std::plus {},
							 parse_identifier,
							 parse_type(parser::accumulate(std::plus {}, parse_attributes, parse_assignment_values),
										parser::accumulate(std::plus {}, parse_attributes, parse_assignment_objects)));

		// we run the parser unbounded times till we hit the end of the view
		return parser::many(parser, return_type {}, std::plus {})(text);
	}
}	 // namespace _details

// calculate the size constraints of the text
constexpr auto size(psl::string8::view text) -> parser::parse_result_t<format_storage_info_t> {
	return _details::parse<_details::size_calculator_t>(text);
}
}	 // namespace psl::serialization::format
