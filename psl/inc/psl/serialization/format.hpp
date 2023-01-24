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
		struct entry {
			size_t count {0};
			size_t size {0};

			friend constexpr entry operator+(entry const& lhs, entry const& rhs) noexcept {
				return entry {.count = lhs.count + rhs.count, .size = lhs.size + rhs.size};
			};
		};

		friend constexpr format_storage_info_t operator+(format_storage_info_t const& lhs,
														 format_storage_info_t const& rhs) noexcept {
			return format_storage_info_t {
			  .directives		 = lhs.directives + rhs.directives,
			  .directives_values = lhs.directives_values + rhs.directives_values,
			  .identifiers		 = lhs.identifiers + rhs.identifiers,
			  .attributes_values = lhs.attributes_values + rhs.attributes_values,
			  .values			 = lhs.values + rhs.values,
			  .prototypes		 = lhs.prototypes + rhs.prototypes,
			  .types			 = lhs.types + rhs.types,
			  .attributes		 = lhs.attributes + rhs.attributes,
			};
		}

		constexpr size_t storage_size() const noexcept {
			return directives.size + directives_values.size + identifiers.size + attributes_values.size + values.size +
				   prototypes.size + types.size + attributes.size;
		}

		entry directives {};
		entry directives_values {};
		entry identifiers {};
		entry attributes_values {};
		entry values {};
		entry prototypes {};
		entry types {0};
		entry attributes {0};
		const char* data {};
		size_t data_size {};
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

	constexpr auto is_prototype(psl::string8::view view) -> bool {
		if(view == "prototype"sv) {
			return true;
		} else if(view.starts_with("prototype"sv)) {
			auto offset = view.find_first_not_of(" \n\r\t"sv, 6);
			if(offset != view.npos && view[offset] == '<') {
				return true;
			}
		}
		return false;
	}


	constexpr auto parse_directive(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
		using namespace psl::serialization::parser;
		constexpr auto parser =
		  skip_whitespace_parser()<parser::text_parser("#"sv) < accumulate_many(none_of(" \n\r\t:"sv))>
		  skip_whitespace_parser();
		return parser(view);
	}

	constexpr auto parse_directive_value(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
		using namespace psl::serialization::parser;
		constexpr auto parser =
		  skip_whitespace_parser()<accumulate_many(none_of(";"sv))> text_parser(";"sv) > skip_whitespace_parser();
		return parser(view);
	}

	constexpr auto parse_identifier(psl::string8::view view)
	  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
		using namespace psl::serialization::parser;
		constexpr auto parser = skip_whitespace_parser()<accumulate(
		  [](auto sv1, auto sv2) {
			  // todo this isn't exactly safe, find better way
			  return psl::string8::view {sv2.data() - sv1.size(), sv1.size() + sv2.size()};
		  },
		  parser::none_of<psl::string8::view>("}#"sv),
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
	  -> psl::serialization::parser::parse_result_t<psl::string8::view> {
		using namespace psl::serialization::parser;
		constexpr auto parser = skip_whitespace_parser() < text_parser(",");
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
	struct storage_entry_t {
		psl::string8::view value {};
		size_t depth {0};
	};

	struct storage_identifier_t : public storage_entry_t {
		size_t value_begin {0};
		size_t value_end {0};
	};

	template <format_storage_info_t Storage>
	struct storage_t {
		psl::static_array<psl::string8::char_t, Storage.storage_size()> storage;
		psl::static_array<storage_entry_t, Storage.identifiers.count> identifiers;
		psl::static_array<storage_entry_t, Storage.values.count> values;
		psl::static_array<storage_entry_t, Storage.attributes.count> attributes;
		psl::static_array<storage_entry_t, Storage.attributes_values.count> attributes_values;
		psl::static_array<storage_entry_t, Storage.types.count> types;
		psl::static_array<storage_entry_t, Storage.prototypes.count> prototypes;
	};

	struct noop_t {
		friend constexpr noop_t operator+(noop_t const&, noop_t const&) noexcept { return {}; }
	};

	template <format_storage_info_t Storage>
	struct storage_writer_t {
		storage_t<Storage> storage {};
		size_t offset {0};
		size_t depth {0};
		using return_type = noop_t;
		constexpr auto transform_directive(psl::string8::view view) -> return_type {
			for(auto c : view) {
				storage.storage[offset++] = c;
			}
			return {};
		}
		constexpr auto transform_directive_value(psl::string8::view view) -> return_type {
			for(auto c : view) {
				storage.storage[offset++] = c;
			}
			return {};
		}

		constexpr auto transform_identifier(psl::string8::view view) -> return_type {
			for(auto c : view) {
				storage.storage[offset++] = c;
			}
			return {};
		}

		constexpr auto transform_type(psl::string8::view view) -> return_type {
			for(auto c : view) {
				storage.storage[offset++] = c;
			}
			return {};
		}

		constexpr auto transform_attribute(psl::string8::view view) -> return_type {
			for(auto c : view) {
				storage.storage[offset++] = c;
			}
			return {};
		}

		constexpr auto transform_attribute_value(psl::string8::view view) -> return_type {
			for(auto c : view) {
				storage.storage[offset++] = c;
			}
			return {};
		}

		constexpr auto transform_value(psl::string8::view view) -> return_type {
			for(auto c : view) {
				storage.storage[offset++] = c;
			}
			return {};
		}

		constexpr auto increase_depth() { ++depth; }
		constexpr auto decrease_depth() { --depth; }
	};

	struct size_calculator_t {
		using return_type = format_storage_info_t;

		constexpr auto transform_directive(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.directives.count = 1;
			info.directives.size  = view.size();
			return info;
		}
		constexpr auto transform_directive_value(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.directives_values.count = 1;
			info.directives_values.size	 = view.size();
			return info;
		}
		constexpr auto transform_identifier(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.identifiers.count = 1;
			info.identifiers.size  = view.size();
			return info;
		}

		constexpr auto transform_type(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.types.count = 1;
			info.types.size	 = view.size();
			if(is_prototype(view)) {
				info.prototypes.size  = view.size();
				info.prototypes.count = 1;
			}
			return info;
		}

		constexpr auto transform_attribute(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.attributes.count = 1;
			info.attributes.size  = view.size();
			return info;
		}

		constexpr auto transform_attribute_value(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.attributes_values.count = 1;
			info.attributes_values.size	 = view.size();
			return info;
		}

		constexpr auto transform_value(psl::string8::view view) -> return_type {
			format_storage_info_t info {};
			info.values.count = 1;
			info.values.size  = view.size();
			return info;
		}

		constexpr auto increase_depth() {}
		constexpr auto decrease_depth() {}
	};

	template <typename T, bool AllowDirectives = true>
	constexpr auto parse(psl::string8::view text, T& target) -> parser::parse_result_t<typename T::return_type> {
		using return_type = typename T::return_type;
		target.increase_depth();

		constexpr auto wrap = [](auto&& fnPtr) {
			return parser::fmap([](psl::string8::view) -> return_type { return return_type {}; }, fnPtr);
		};

		auto wrap_apply = [&target](auto&& fnPtr, auto&& memFnPtr) {
			return parser::fmap(
			  [memFnPtr, &target](psl::string8::view view) -> return_type { return (target.*memFnPtr)(view); }, fnPtr);
		};

		// parse directive statements
		auto parse_directive =
		  parser::accumulate(std::plus {},
							 wrap_apply(_details::parse_directive, &T::transform_directive),
							 wrap_apply(_details::parse_directive_value, &T::transform_directive_value));

		// parse until the type seperator
		auto parse_identifier = parser::accumulate(std::plus {},
												   wrap_apply(_details::parse_identifier, &T::transform_identifier),
												   wrap(_details::parse_identifier_type_seperator));

		// parse the type until the end of a template statement is found, or the start of an attribute or assignment
		auto parse_type = wrap_apply(_details::parse_type, &T::transform_type);

		// parse all attributes until no more can be found
		auto parse_attributes = parser::many(parser::accumulate(
		  std::plus {},
		  wrap(_details::parse_attribute_begin),
		  wrap_apply(_details::parse_attribute_identfier, &T::transform_attribute),
		  parser::accumulate(std::plus {},
							 wrap(_details::parse_attribute_assignment_begin),
							 wrap_apply(_details::parse_attribute_assignment_value, &T::transform_attribute_value),
							 wrap(_details::parse_attribute_assignment_end),
							 wrap(_details::parse_attribute_end)) |
			wrap(_details::parse_attribute_end)));

		// parses all values until no seperators can be found
		auto parse_assignment_values = parser::accumulate(
		  std::plus {},
		  wrap_apply(_details::parse_assigment_value, &T::transform_value),
		  parser::many(parser::accumulate(std::plus {},
										  wrap(_details::parse_assigment_value_more),
										  wrap_apply(_details::parse_assigment_value, &T::transform_value))));

		// parses all objects until the end statement is found.
		// we first scan to see if the next item would satisfy an object (i.e. does it have an identifier and a split)
		// otherwise we invalidate the search. For the loop we internally first scan for the end statement to decide if
		// a next object even exists.
		auto parse_assignment_objects = [&target](parser::parse_view_t view) -> parser::parse_result_t<return_type> {
			constexpr auto parse_identifier =
			  wrap(_details::parse_identifier) < wrap(_details::parse_identifier_type_seperator);
			if(const auto is_identifier = parse_identifier(view); is_identifier) {
				return parser::many([&target](parser::parse_view_t view) -> parser::parse_result_t<return_type> {
					if(const auto end = _details::parse_assignment_end(view); end) {
						return parser::invalid_result;
					}
					return parse<std::remove_cvref_t<decltype(target)>, false>(view, target);
				})(view);
			}
			return parser::invalid_result;
		};

		// the combination of all previous parsers into a continuous statement that recursively scans the fields
		auto parse_field =
		  parser::accumulate(std::plus {},
							 parse_identifier,
							 parse_type,
							 parse_attributes,
							 wrap(_details::parse_assignment_begin),
							 wrap(_details::parse_assignment_end) | parse_assignment_objects | parse_assignment_values,
							 wrap(_details::parse_assignment_end));

		// we run the parser unbounded times till we hit the end of the view
		// if we are root, we allow directives to be scanned, otherwise don't.
		if constexpr(AllowDirectives) {
			const auto result = parser::many(parse_directive | parse_field)(text);
			target.decrease_depth();
			return result;
		} else {
			const auto result = parser::many(parse_field)(text);
			target.decrease_depth();
			return result;
		}
	}
}	 // namespace _details

// calculate the size constraints of the text
constexpr auto size(psl::string8::view text) -> parser::parse_result_t<format_storage_info_t> {
	_details::size_calculator_t calculator {};
	auto result = _details::parse(text, calculator);
	if(result) {
		result.value().data		 = text.data();
		result.value().data_size = text.size();
	}
	return result;
}

template <auto View>
consteval auto parse(psl::ct_string_wrapper<View>) {
	constexpr auto calculated_size = size(View);
	_details::storage_writer_t<calculated_size.value()> writer {};
	_details::parse(View, writer);
	return writer.storage;
}

template <format_storage_info_t Storage>
constexpr auto parse() {
	constexpr psl::string8::view text = {Storage.data, Storage.data_size};
	_details::storage_writer_t<Storage> writer {};
	_details::parse(text, writer);
	return writer.storage;
}
}	 // namespace psl::serialization::format
