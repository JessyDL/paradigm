#pragma once

#include "psl/collections/compile_time_string.hpp"
#include "psl/serialization/parser.hpp"
#include <algorithm>
#include <span>
#include <stdexcept>
#include <strtype/strtype.hpp>

namespace psl::serialization::format {
constexpr std::uint64_t parse_to(psl::string8::view view) {
	std::uint64_t result {0};
	size_t magnitude {1};
	for(size_t i = 0; i < view.size(); ++i) {
		result += view[view.size() - (i + 1)] * magnitude;
		magnitude *= 10;
	}
	return result;
}

template <typename T>
concept HasParser = requires(psl::string8::view view) {
	{ parse_to(view) } -> std::same_as<T>;
};

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
			  .max_depth		 = std::max(lhs.max_depth, rhs.max_depth),
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
		size_t data_size {0};
		size_t max_depth {0};
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

	struct storage_field_t {
		enum class field_type : std::uint8_t {
			unknown = 0,
			directive,
			identifier,
			attribute,
			type,
			prototype,
			directive_value,
			attribute_value,
			value,
		};

		// todo: msvc has an ICE that means we can't just use a psl::string8::view like we should be able to
		// accessing the beginning of storage's data (char const*) results in the ICE, but is only triggered
		// under certain conditions.
		// sadly this means we are forced to always access the entry's information through the storage that
		// can transform this begin/end into something useful.
		struct storage_entry_view_t {
			size_t begin {0};
			size_t end {0};
		} value {};

		size_t depth {0};
		field_type type {field_type::unknown};
	};

	template <format_storage_info_t Storage>
	struct storage_t {
		constexpr auto data() const noexcept { return storage.data(); }
		constexpr auto size() const noexcept { return Storage.storage_size(); }

		constexpr auto view() const noexcept -> psl::string8::view { return {data(), size()}; }

		constexpr auto max_depth() const noexcept -> size_t { return Storage.max_depth; }

		constexpr auto name(storage_field_t const& field) const noexcept -> psl::string8::view {
			return {storage.data() + field.value.begin, field.value.end - field.value.begin};
		}

		psl::static_array<psl::string8::char_t, Storage.storage_size()> storage;
		psl::static_array<storage_field_t,
						  Storage.directives.count + Storage.directives_values.count + Storage.identifiers.count +
							Storage.values.count + Storage.attributes.count + Storage.attributes_values.count +
							Storage.types.count + Storage.prototypes.count>
		  fields {};
	};

	struct noop_t {
		friend constexpr noop_t operator+(noop_t const&, noop_t const&) noexcept { return {}; }
	};

	template <format_storage_info_t Storage>
	struct storage_writer_t {
		storage_t<Storage> storage;
		size_t offset {0};
		size_t depth {0};

		size_t field_index {0};

		using return_type = noop_t;

		constexpr auto write(psl::string8::view view, storage_field_t::field_type type) {
			auto begin = offset;
			for(auto c : view) {
				storage.storage[offset++] = c;
			}
			storage.fields[field_index++] = {.value = {begin, offset}, .depth = depth, .type = type};
		}

		constexpr auto transform_directive(psl::string8::view view) -> return_type {
			write(view, storage_field_t::field_type::directive);
			return {};
		}
		constexpr auto transform_directive_value(psl::string8::view view) -> return_type {
			write(view, storage_field_t::field_type::directive_value);
			return {};
		}

		constexpr auto transform_identifier(psl::string8::view view) -> return_type {
			write(view, storage_field_t::field_type::identifier);
			return {};
		}

		constexpr auto transform_type(psl::string8::view view) -> return_type {
			write(view, storage_field_t::field_type::type);
			return {};
		}

		constexpr auto transform_attribute(psl::string8::view view) -> return_type {
			write(view, storage_field_t::field_type::attribute);
			return {};
		}

		constexpr auto transform_attribute_value(psl::string8::view view) -> return_type {
			write(view, storage_field_t::field_type::attribute_value);
			return {};
		}

		constexpr auto transform_value(psl::string8::view view) -> return_type {
			write(view, storage_field_t::field_type::value);
			return {};
		}

		constexpr auto increase_depth() { ++depth; }
		constexpr auto decrease_depth() { --depth; }
	};

	struct size_calculator_t {
		using return_type = format_storage_info_t;
		size_t depth {0};
		size_t max_depth {0};

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

		constexpr auto increase_depth() noexcept {
			++depth;
			max_depth = std::max(max_depth, depth);
		}
		constexpr auto decrease_depth() noexcept { --depth; }
	};

	template <typename T, bool AllowDirectives = true>
	constexpr auto parse(psl::string8::view text, T& target) -> parser::parse_result_t<typename T::return_type> {
		using return_type = typename T::return_type;

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
		// we increase the scope depth internally
		auto parse_assignment_values = parser::accumulate(
		  std::plus {},
		  [&target, &wrap_apply](parser::parse_view_t view) {
			  target.increase_depth();
			  return wrap_apply(_details::parse_assigment_value, &T::transform_value)(view);
		  },
		  [&target, &wrap, &wrap_apply](parser::parse_view_t view) {
			  const auto result = parser::many(
				parser::accumulate(std::plus {},
								   wrap(_details::parse_assigment_value_more),
								   wrap_apply(_details::parse_assigment_value, &T::transform_value)))(view);
			  target.decrease_depth();
			  return result;
		  });

		// parses all objects until the end statement is found.
		// we first scan to see if the next item would satisfy an object (i.e. does it have an identifier and a split)
		// otherwise we invalidate the search. For the loop we internally first scan for the end statement to decide if
		// a next object even exists.
		auto parse_assignment_objects = [&target](parser::parse_view_t view) -> parser::parse_result_t<return_type> {
			constexpr auto parse_identifier =
			  wrap(_details::parse_identifier) < wrap(_details::parse_identifier_type_seperator);
			if(const auto is_identifier = parse_identifier(view); is_identifier) {
				target.increase_depth();
				const auto result =
				  parser::many([&target](parser::parse_view_t view) -> parser::parse_result_t<return_type> {
					  if(const auto end = _details::parse_assignment_end(view); end) {
						  return parser::invalid_result;
					  }
					  return parse<std::remove_cvref_t<decltype(target)>, false>(view, target);
				  })(view);
				target.decrease_depth();
				return result;
			}
			return parser::invalid_result;
		};

		// the combination of all previous parsers into a continuous statement that recursively scans the fields
		auto parse_field = parser::accumulate(std::plus {},
											  parse_identifier,
											  parse_type,
											  parse_attributes,
											  wrap(_details::parse_assignment_begin),
											  parse_assignment_objects | parse_assignment_values,
											  wrap(_details::parse_assignment_end));

		// we run the parser unbounded times till we hit the end of the view
		// if we are root, we allow directives to be scanned, otherwise don't.
		if constexpr(AllowDirectives) {
			const auto result = parser::many(parse_directive | parse_field)(text);
			return result;
		} else {
			const auto result = parser::many(parse_field)(text);
			return result;
		}
	}

	constexpr size_t field_count(auto const& storage, size_t index = 0) {
		constexpr std::array valid_field_types {storage_field_t::field_type::identifier,
												storage_field_t::field_type::value};
		auto target = std::next(std::begin(storage.fields), index);
		if(target->type != storage_field_t::field_type::identifier) {
			throw std::runtime_error("target wasn't an identifier");
		}

		// forward scan to one beyond the type or attributes, whichever was last
		constexpr auto skip_invalid_fields = [](auto target) {
			do {
				++target;
			} while(std::find(std::begin(valid_field_types), std::end(valid_field_types), target->type) ==
					std::end(valid_field_types));
			return target;
		};
		size_t values {0};


		for(auto it = skip_invalid_fields(target); it != std::end(storage.fields) && target->depth < it->depth; ++it) {
			if(it->depth == target->depth + 1 &&
			   std::find(std::begin(valid_field_types), std::end(valid_field_types), it->type) !=
				 std::end(valid_field_types)) {
				++values;
			}
		}
		return values;
	}

	template <size_t Depth, auto Storage>
	constexpr size_t fields_at_depth() noexcept {
		auto next_field_at_depth = [](auto iterator) {
			auto next_field_at_depth = [](auto const& field) {
				return field.type == storage_field_t::field_type::identifier && field.depth == Depth;
			};
			return std::find_if(iterator, std::end(Storage.fields), next_field_at_depth);
		};

		size_t count {0};
		for(auto it = next_field_at_depth(std::begin(Storage.fields)); it != std::end(Storage.fields);
			it		= next_field_at_depth(it + 1)) {
			++count;
		}
		return count;
	}

	struct ct_format_value_t {
		psl::string8::view value;
	};

	struct ct_format_field_t {
		psl::string8::view name;
		psl::string8::view type;
		std::span<ct_format_field_t> children {};
		std::span<ct_format_value_t> values {};
	};

	template <size_t Size>
	struct ct_format_layer_t {
		psl::static_array<ct_format_field_t, Size> fields {};
	};

	// ...Sizes is a template pack that signifies the amount of fields to expect per layer
	template <size_t... Sizes>
	struct ct_format_t {
		std::tuple<ct_format_layer_t<Sizes>...> layers {};
	};

	template <size_t Depth, auto Storage>
	constexpr auto fill_names(auto& fields) {
		auto next_field_at_depth = [](auto iterator) {
			auto next_field_at_depth = [](auto const& field) {
				return field.type == storage_field_t::field_type::identifier && field.depth == Depth;
			};
			return std::find_if(iterator, std::end(Storage.fields), next_field_at_depth);
		};

		auto target = std::begin(fields);

		for(auto it = next_field_at_depth(std::begin(Storage.fields)); it != std::end(Storage.fields);
			it		= next_field_at_depth(it + 1)) {
			target->name = Storage.name(*it);
			target->type = Storage.name(*std::next(it));
			target		 = std::next(target);
		}
	}

	template <size_t MaxDepth, auto Storage>
	constexpr auto transform_intermediate_to_user_format() {
		auto result = []<size_t... Depth>(std::index_sequence<Depth...>) {
			auto result = ct_format_t<fields_at_depth<Depth, Storage>()...> {};

			([](auto& layer) { fill_names<Depth, Storage>(layer.fields); }(std::get<Depth>(result.layers)), ...);

			return result;
		}
		(std::make_index_sequence<MaxDepth>());

		return result;
	}
}	 // namespace _details

// calculate the size constraints of the text
constexpr auto size(psl::string8::view text) -> parser::parse_result_t<format_storage_info_t> {
	_details::size_calculator_t calculator {};
	auto result = _details::parse(text, calculator);
	if(result) {
		result.value().data		 = text.data();
		result.value().data_size = text.size();
		result.value().max_depth = calculator.max_depth;
	}
	return result;
}

template <format_storage_info_t Storage>
constexpr auto parse() {
	// write the format_storage_info_t into an intermediate storage container
	// from there we can form the tree in a user-friendly manner and start the process
	// of validating the attributes and types.
	constexpr auto intermediate_storage = []() {
		_details::storage_writer_t<Storage> writer {};
		auto result = _details::parse({Storage.data, Storage.data_size}, writer);
		return writer.storage;
	}();

	return transform_intermediate_to_user_format<Storage.max_depth, intermediate_storage>();
}

template <auto View>
consteval auto parse(psl::ct_string_wrapper<View>) {
	constexpr auto calculated_size = size(View);
	_details::storage_writer_t<calculated_size.value()> writer {};
	_details::parse(View, writer);
	return parse<writer.storage>();
}
}	 // namespace psl::serialization::format
