#pragma once
#include "psl/array.hpp"
#include "psl/assertions.hpp"
#include "psl/static_array.hpp"
#include "psl/ustring.hpp"

#include "psl/string_utils.hpp"

#include <fmt/format.h>

namespace psl::serialization {
namespace constants {
	static constexpr psl::string8::view EMPTY_CHARACTERS		   = " \n\t\r ";
	static constexpr psl::string8::view IDENTIFIER_DIVIDER		   = ":";
	static constexpr psl::string8::view IDENTIFIER_ASSIGNMENT	   = "=";
	static constexpr psl::string8::view IDENTIFIER_ATTRIBUTE_BEGIN = "[";
	static constexpr psl::string8::view IDENTIFIER_ATTRIBUTE_END   = "]";
	static constexpr psl::string8::view IDENTIFIER_VALUE_BEGIN	   = "{";
	static constexpr psl::string8::view IDENTIFIER_VALUE_END	   = "}";
	static constexpr psl::string8::view IDENTIFIER_ARRAY_SEPERATOR = ",";
	static constexpr psl::string8::view CLOSE_STATEMENT			   = ";";
	static constexpr psl::string8::view LITERALS				   = "\"'";
	static constexpr psl::string8::view OPEN_TEMPLATE			   = "<";
	static constexpr psl::string8::view CLOSE_TEMPLATE			   = ">";

	static constexpr psl::string8::view VALID_IDENTIFIER_LEADING_CHARACTERS =
	  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
	static constexpr psl::string8::view VALID_IDENTIFIER_CHARACTERS =
	  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
}	 // namespace constants

inline namespace _format_details {
	struct error_line_info_t {
		constexpr error_line_info_t(psl::string8::view content, size_t offset) : content(content), offset(offset) {};

		operator std::string() const noexcept {
			auto line_begin = content.rfind('\n', offset);

			if(line_begin != psl::string8::view::npos) {
				auto relative_offset = offset - line_begin;
				size_t lines {1};
				while(line_begin != psl::string8::view::npos) {
					line_begin = content.rfind('\n', line_begin);
					++lines;
				}
				return fmt::format("at line {}:{}", lines, relative_offset);
			}
			return fmt::format("at line 1:{}", offset);
		}

		psl::string8::view content;
		size_t offset;
	};

	enum class parser_type_e : std::uint8_t {
		value		 = 0,
		array		 = 1 << 0,
		object		 = 1 << 1,
		prototype	 = 1 << 2,
		object_array = array + object,
	};

	struct parser_attribute_t {
		psl::string8::view name;
		psl::string8::view content;
	};

	struct parser_type_t {
		psl::string8::view name;
		psl::array<parser_attribute_t> attributes;
		parser_type_e type;
	};

	struct parser_value_t {
		psl::string8::view content;
	};

	struct parser_identifier_t {
		psl::string8::view name;
		parser_type_t type;
		psl::array<parser_value_t> values		 = {};
		psl::array<parser_identifier_t> children = {};
	};

	struct parser_prototype_t {
		psl::string name;
		psl::array<parser_attribute_t> attributes;
		psl::array<parser_identifier_t> children = {};
	};

	struct parser_directive_t {
		psl::string8::view name;
		psl::string8::view value;
	};

	struct typecache_t {
		struct entry {
			psl::string name;
			psl::array<parser_attribute_t> attributes;
			psl::array<parser_identifier_t> children = {};
		};

		psl::array<entry> m_Entries;
	};

	constexpr auto decode_attributes(psl::string8::view content, size_t& offset) -> parser_attribute_t {
		return parser_attribute_t {};
	}

	constexpr auto decode_type(psl::string8::view content, size_t& offset, typecache_t& typecache) -> parser_type_t {
		// find the first non empty character so we know where the type info starts
		auto type_begin = content.find_first_not_of(constants::EMPTY_CHARACTERS, offset);
		psl::assertion([&type_begin]() { return type_begin != psl::string8::view::npos; },
					   "Could not find the begining of the identifier type assignment.\n{}",
					   error_line_info_t {content, offset});

		// find the end of the sequence
		auto index = content.find(constants::IDENTIFIER_ASSIGNMENT, type_begin);
		if(index == psl::string8::view::npos) {
			index = content.find(constants::CLOSE_STATEMENT, type_begin);
		}

		psl::assertion(
		  [&index]() { return index != psl::string8::view::npos; },
		  "Could not find the end of the identifier type assignment. Did you forget to add an `{}` or `{}`?\n{}",
		  constants::IDENTIFIER_ASSIGNMENT,
		  constants::CLOSE_STATEMENT,
		  error_line_info_t {content, type_begin});
		offset = index + 1;


		auto attribute_begin = content.find(constants::IDENTIFIER_ATTRIBUTE_BEGIN, offset);
		auto type_end =
		  utility::string::rfind_first_not_of(content,
											  constants::EMPTY_CHARACTERS,
											  (attribute_begin == psl::string8::view::npos) ? index : attribute_begin);
		psl::array<parser_attribute_t> attributes {};
		while(attribute_begin != psl::string8::view::npos) {
			attribute_begin += 1;
			attributes.emplace_back(std::move(decode_attributes(content, attribute_begin)));
			attribute_begin = content.find(constants::IDENTIFIER_ATTRIBUTE_BEGIN, attribute_begin);
		}
		auto name		   = content.substr(type_begin, type_end - type_begin);
		parser_type_e type = parser_type_e::value;

		auto is_keyword = [](psl::string8::view name,
							 psl::string8::view keyword,
							 psl::string8::view end_tokens = constants::CLOSE_TEMPLATE,
							 size_t* out				   = nullptr) -> bool {
			if(name.starts_with(keyword)) {
				if(name.size() == keyword.size()) {
					if(out) {
						*out = name.size();
					}
					return true;
				}
				auto next = name.find_first_not_of(constants::EMPTY_CHARACTERS, keyword.size());
				if(next == psl::string8::view::npos || next != keyword.size() ||
				   end_tokens.find(name[next]) != psl::string8::view::npos) {
					if(out) {
						*out = (next != psl::string8::view::npos) ? next : name.size();
					}
					return true;
				}
			}
			return false;
		};

		size_t name_next = 0;
		if(is_keyword(name, "prototype", constants::OPEN_TEMPLATE, &name_next)) {
			type = parser_type_e(static_cast<std::underlying_type_t<parser_type_e>>(type) |
								 static_cast<std::underlying_type_t<parser_type_e>>(parser_type_e::prototype));
			name_next += 1;
			auto next_end = name.find(constants::CLOSE_TEMPLATE, name_next);


		} else {
			if(is_keyword(name, "array", constants::OPEN_TEMPLATE, &name_next)) {
				type = parser_type_e(static_cast<std::underlying_type_t<parser_type_e>>(type) |
									 static_cast<std::underlying_type_t<parser_type_e>>(parser_type_e::array));
				name_next += 1;
			} 
			if(is_keyword(name.substr(name_next), "object") ||
				std::any_of(std::begin(typecache.m_Entries),
							std::end(typecache.m_Entries),
							[&is_keyword, name = name.substr(name_next)](const auto& entry) {
								return is_keyword(name, entry.name);
							})) {
			type = parser_type_e(static_cast<std::underlying_type_t<parser_type_e>>(type) |
									static_cast<std::underlying_type_t<parser_type_e>>(parser_type_e::object));
			}
		}

		return parser_type_t {name, attributes, type};
	}

	constexpr auto decode_object(psl::string8::view content, size_t& offset) -> parser_identifier_t {
		return parser_identifier_t {};
	}

	constexpr auto decode_prototype(psl::string8::view content, size_t& offset, typecache_t& typecache) {}

	constexpr auto decode_values(psl::string8::view content, size_t& offset) -> psl::array<parser_value_t> {
		auto begin_value = content.find(constants::IDENTIFIER_VALUE_BEGIN, offset);
		psl::assertion(
		  [&begin_value]() { return begin_value != psl::string8::view::npos; },
		  "Could not find the start of the identifier's value assignment. Did you forget to add an `{}`?\n{}",
		  constants::IDENTIFIER_VALUE_BEGIN,
		  error_line_info_t {content, offset});
		begin_value		= content.find_first_not_of(constants::EMPTY_CHARACTERS, begin_value + 1);
		auto next_value = [](psl::string8::view content, size_t begin) -> size_t {
			psl::string8::view current_literal {};
			auto end_value = psl::string8::view::npos;
			for(auto index = begin; index != content.size(); ++index) {
				if(current_literal.empty()) {
					if(content[index] == constants::IDENTIFIER_VALUE_END[0] ||
					   content[index] == constants::IDENTIFIER_ARRAY_SEPERATOR[0]) {
						end_value = index;
						break;
					} else if(constants::LITERALS.find(content[index]) != psl::string8::view::npos) {
						if(content.size() > index + 2 && content[index + 1] == content[index] &&
						   content[index + 2] == content[index]) {
							current_literal = content.substr(index, 3);
							index += 2;
						} else {
							current_literal = content.substr(index, 1);
						}
						continue;
					}
				} else if(content[index] == current_literal[0]) {
					if(current_literal.size() == 1 || current_literal.size() == 3 && content.size() > index + 2 &&
														current_literal[1] == content[index + 1] &&
														current_literal[2] == content[index + 2]) {
						index += current_literal.size() - 1;
						current_literal = {};
					}
				}
			}
			return end_value;
		};

		auto trim = [](psl::string8::view content, size_t begin, size_t end) -> psl::string8::view {
			begin = content.find_first_not_of(constants::EMPTY_CHARACTERS, begin);
			end	  = utility::string::rfind_first_not_of(content, constants::EMPTY_CHARACTERS, end);
			if(constants::LITERALS.find(content[begin]) != psl::string8::view::npos &&
			   content[begin] == content[end - 1]) {
				++begin, --end;
				if(content[begin - 1] == content[begin] && content[begin + 1] == content[begin] &&
				   content[end - 2] == content[end] && content[end - 1] == content[end]) {
					begin += 2;
					end -= 2;
				}
			}
			return content.substr(begin, end - begin);
		};

		psl::array<parser_value_t> results {};
		auto next {psl::string8::view::npos};
		do {
			next = next_value(content, begin_value);
			results.emplace_back(trim(content, begin_value, next));
			begin_value = next + 1;
		} while(next != psl::string8::view::npos && content[next] == constants::IDENTIFIER_ARRAY_SEPERATOR[0]);

		psl::assertion(
		  [&next]() { return next != psl::string8::view::npos; },
		  "Could not find the end of the identifier's value assignment. Did you forget to add an `{}`?\n{}",
		  constants::IDENTIFIER_VALUE_END,
		  error_line_info_t {content, offset});

		auto close_statement_index = content.find(constants::CLOSE_STATEMENT, next + 1);

		psl::assertion([&close_statement_index]() { return close_statement_index != psl::string8::view::npos; },
					   "Could not find the close statement. Did you forget to add an `{}`?\n{}",
					   constants::CLOSE_STATEMENT,
					   error_line_info_t {content, offset});

		offset = close_statement_index + 1;
		return results;
	}

	constexpr auto decode_identifier(psl::string8::view content, size_t& offset, typecache_t& typecache)
	  -> parser_identifier_t {
		auto identifier_begin = content.find_first_not_of(constants::EMPTY_CHARACTERS, offset);
		psl::assertion([&identifier_begin]() { return identifier_begin != psl::string8::view::npos; },
					   "Could not find the beginning of the identifier. Did you forget to add an `{}`?\n{}",
					   constants::IDENTIFIER_DIVIDER,
					   error_line_info_t {content, offset});
		auto identifier_end = content.find(constants::IDENTIFIER_DIVIDER, identifier_begin);
		psl::assertion([&identifier_end]() { return identifier_end != psl::string8::view::npos; },
					   "Could not find the end of the identifier. Did you forget to add an `{}`?\n{}",
					   constants::IDENTIFIER_DIVIDER,
					   error_line_info_t {content, offset});

		offset = identifier_end + 1;

		identifier_end = utility::string::rfind_first_not_of(content, constants::EMPTY_CHARACTERS, identifier_end);
		psl::assertion(
		  [&identifier_end, &identifier_begin]() {
			  return identifier_end != psl::string8::view ::npos && identifier_end != identifier_begin;
		  },
		  "Identifier is unknown. Please assign a name.\n{}",
		  error_line_info_t {content, offset});

		auto identifier = content.substr(identifier_begin, identifier_end - identifier_begin);
		auto type		= decode_type(content, offset, typecache);

		switch(type.type) {
		case parser_type_e::value:
		case parser_type_e::array:
			return parser_identifier_t {identifier, type, {decode_values(content, offset)}};
		case parser_type_e::object:
			return parser_identifier_t {identifier, type, {}, {decode_object(content, offset)}};
		case parser_type_e::object_array:
			return parser_identifier_t {identifier, type, {}, {decode_object(content, offset)}};
		case parser_type_e::prototype:
			decode_prototype(content, offset, typecache);
			return parser_identifier_t {};
		}
		psl::unreachable("unknown value was in parser_type_e");
	}

	constexpr auto decode_directive(psl::string8::view content, size_t& offset) -> parser_directive_t {
		auto directive_begin = content.find_first_not_of(constants::EMPTY_CHARACTERS, offset);
		psl::assertion([&directive_begin]() { return directive_begin != psl::string8::view::npos; },
					   "Incorrect directive statement.\n{}",
					   error_line_info_t {content, offset});
		auto directive_end = content.find_first_of(constants::EMPTY_CHARACTERS, directive_begin);
		psl::assertion([&directive_end]() { return directive_end != psl::string8::view::npos; },
					   "Incorrect directive statement.\n{}",
					   error_line_info_t {content, offset});
		auto value_begin = content.find_first_not_of(constants::EMPTY_CHARACTERS, directive_end);
		psl::assertion([&value_begin]() { return value_begin != psl::string8::view::npos; },
					   "Incorrect directive statement.\n{}",
					   error_line_info_t {content, offset});
		auto value_end = content.find(constants::CLOSE_STATEMENT, value_begin);
		psl::assertion([&value_end]() { return value_end != psl::string8::view::npos; },
					   "Incorrect directive statement.\n{}",
					   error_line_info_t {content, offset});
		offset = value_end + 1;
		return parser_directive_t {content.substr(directive_begin, directive_end - directive_begin),
								   content.substr(value_begin, value_end - value_begin)};
	}
}	 // namespace _format_details

class format_t {
	struct header_t {
		static constexpr psl::string8::view HEADER_IDENTIFIER = "PFHF";
	};


  public:
	enum class type_t : uint8_t {
		MALFORMED		= 0,
		VALUE			= 1,
		VALUE_RANGE		= 2,
		REFERENCE		= 3,
		REFERENCE_RANGE = 4,
		COLLECTION		= 5
	};

	format_t() noexcept										= default;
	constexpr ~format_t()									= default;
	constexpr format_t(const format_t&) noexcept			= default;
	constexpr format_t(format_t&&) noexcept					= default;
	constexpr format_t& operator=(const format_t&) noexcept = default;
	constexpr format_t& operator=(format_t&&) noexcept		= default;

	format_t(psl::string8::view content) noexcept { parse(content); }

	constexpr auto parse(psl::string8::view content) -> void {
		size_t offset {content.find_first_not_of(constants::EMPTY_CHARACTERS)};
		psl::array<parser_directive_t> directives {};
		psl::array<parser_identifier_t> identifiers {};
		typecache_t typecache = m_TypeCache;
		while(offset != psl::string8::view::npos && offset < content.size()) {
			if(content[offset] == '#') {
				offset += 1;
				directives.emplace_back(decode_directive(content, offset));
			} else {
				identifiers.emplace_back(decode_identifier(content, offset, typecache));
			}
			offset = content.find_first_not_of(constants::EMPTY_CHARACTERS, offset);
		}
		m_Directives  = directives;
		m_Identifiers = identifiers;
		m_TypeCache	  = typecache;
	}

  private:
	constexpr auto next_token(psl::string8::view content,
							  size_t offset = 0,
							  size_t end	= psl::string8::view::npos) const noexcept -> size_t {
		auto index = content.find_first_not_of(constants::EMPTY_CHARACTERS, offset);
		if(index == psl::string8::view::npos || index >= end) {
			return content.size();
		}
		return index;
	}
	psl::array<parser_directive_t> m_Directives {};
	psl::array<parser_identifier_t> m_Identifiers {};
	typecache_t m_TypeCache {};
};

using format_type_t = format_t::type_t;
}	 // namespace psl::serialization
