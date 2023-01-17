#pragma once

// #include <string>
#include "psl/ustring.hpp"
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <memory>
#include <numeric>
#include <string_view>
#include <vector>
#if __has_include(<charconv>)
	#include <charconv>
#endif
#include "psl/template_utils.hpp"


namespace utility {
class string_constructor_t {
	psl::string8::view value;
};

/// \brief utility namespace to deal with psl::string operations.
namespace string {
	/// \brief convert the given input string to capital letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string8_t to_upper(psl::string8::view str) {
		psl::string8_t result {str};
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		return result;
	}

	/// \brief convert the given input string to capital letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string16_t to_upper(psl::string16::view str) {
		psl::string16_t result {str};
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		return result;
	}

	/// \brief convert the given input string to capital letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string32_t to_upper(psl::string32::view str) {
		psl::string32_t result {str};
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		return result;
	}

	/// \brief convert the given input string to capital letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string8_t& to_upper(psl::string8_t& str) {
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}

	/// \brief convert the given input string to capital letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string16_t& to_upper(psl::string16_t& str) {
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}

	/// \brief convert the given input string to capital letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string32_t& to_upper(psl::string32_t& str) {
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}

	/// \brief convert the given input string to lowercase letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string8_t to_lower(psl::string8::view str) {
		psl::string8_t result {str};
		std::transform(result.begin(), result.end(), result.begin(), ::tolower);
		return result;
	}

	/// \brief convert the given input string to lowercase letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string16_t to_lower(psl::string16::view str) {
		psl::string16_t result {str};
		std::transform(result.begin(), result.end(), result.begin(), ::tolower);
		return result;
	}

	/// \brief convert the given input string to lowercase letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string32_t to_lower(psl::string32::view str) {
		psl::string32_t result {str};
		std::transform(result.begin(), result.end(), result.begin(), ::tolower);
		return result;
	}


	/// \brief convert the given input string to lowercase letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string8_t& to_lower(psl::string8_t& str) {
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}

	/// \brief convert the given input string to lowercase letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string16_t& to_lower(psl::string16_t& str) {
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}

	/// \brief convert the given input string to lowercase letters (if possible).
	/// \param[in] str the string to transform.
	/// \returns the transformed string.
	inline psl::string32_t& to_lower(psl::string32_t& str) {
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}

	inline std::vector<psl::string8::view>& split(psl::string8::view s,
												  psl::string8::view delimiter,
												  std::vector<psl::string8::view>& inout,
												  bool ignore_consecutive = false) {
		size_t pos	= 0u;
		size_t prev = 0u;
		if(ignore_consecutive) {
			while((pos = s.find(delimiter, pos)) != psl::string8::view::npos) {
				inout.emplace_back(s.substr(prev, pos - prev));
				do {
					pos += delimiter.length();
				} while(s.substr(pos, delimiter.length()) == delimiter);
				prev = pos;
			}
		} else {
			while((pos = s.find(delimiter, pos)) != psl::string8::view::npos) {
				inout.emplace_back(s.substr(prev, pos - prev));
				pos += delimiter.length();
				prev = pos;
			}
		}
		pos = s.size();
		inout.emplace_back(s.substr(prev, pos - prev));
		return inout;
	}
	inline std::vector<psl::string8::view>
	split(psl::string8::view s, psl::string8::view delimiter, bool ignore_consecutive = true) {
		std::vector<psl::string8::view> res;
		split(s, delimiter, res, ignore_consecutive);
		return res;
	}

	inline std::vector<psl::string16::view>&
	split(psl::string16::view s, psl::string16::view delimiter, std::vector<psl::string16::view>& inout) {
		size_t pos	= 0u;
		size_t prev = 0u;
		while((pos = s.find(delimiter, pos)) != psl::string16::view::npos) {
			inout.emplace_back(s.substr(prev, pos - prev));
			pos += delimiter.length();
			prev = pos;
		}
		pos = s.size();
		inout.emplace_back(s.substr(prev, pos - prev));
		return inout;
	}
	inline std::vector<psl::string16::view> split(psl::string16::view s, psl::string16::view delimiter) {
		std::vector<psl::string16::view> res;
		split(s, delimiter, res);
		return res;
	}


	inline std::vector<psl::string32::view>&
	split(psl::string32::view s, psl::string32::view delimiter, std::vector<psl::string32::view>& inout) {
		size_t pos	= 0u;
		size_t prev = 0u;
		while((pos = s.find(delimiter, pos)) != psl::string32::view::npos) {
			inout.emplace_back(s.substr(prev, pos - prev));
			pos += delimiter.length();
			prev = pos;
		}
		pos = s.size();
		inout.emplace_back(s.substr(prev, pos - prev));
		return inout;
	}
	inline std::vector<psl::string32::view> split(psl::string32::view s, psl::string32::view delimiter) {
		std::vector<psl::string32::view> res;
		split(s, delimiter, res);
		return res;
	}

	inline size_t size(const std::vector<psl::string8::view>& values) {
		using type = size_t;
		type chars =
		  std::accumulate(std::begin(values), std::end(values), type(0), [](type total, const psl::string8::view& s) {
			  return total + s.length();
		  });
		return chars;
	}

	inline size_t size(const std::vector<psl::string16::view>& values) {
		using type = size_t;
		type chars =
		  std::accumulate(std::begin(values), std::end(values), type(0), [](type total, const psl::string16::view& s) {
			  return total + s.length();
		  });
		return chars;
	}

	inline size_t size(const std::vector<psl::string32::view>& values) {
		using type = size_t;
		type chars =
		  std::accumulate(std::begin(values), std::end(values), type(0), [](type total, const psl::string32::view& s) {
			  return total + s.length();
		  });
		return chars;
	}


	inline psl::string16_t& erase_consecutive(psl::string16_t& str, psl::string16::view consecutive) {
		size_t pos = 0;
		while((pos = str.find(consecutive)) != psl::string16::view::npos) {
			while(str.substr(pos + consecutive.size(), consecutive.size()) == consecutive) {
				str.erase(std::begin(str) + pos + consecutive.size(), std::begin(str) + pos + consecutive.size() * 2);
			}
		}
		return str;
	}


	inline psl::string32_t& erase_consecutive(psl::string32_t& str, psl::string32::view consecutive) {
		size_t pos = 0;
		while((pos = str.find(consecutive)) != psl::string32::view::npos) {
			while(str.substr(pos + consecutive.size(), consecutive.size()) == consecutive) {
				str.erase(std::begin(str) + pos + consecutive.size(), std::begin(str) + pos + consecutive.size() * 2);
			}
		}
		return str;
	}

	inline size_t contains(psl::string8::view s, psl::string8::view search) {
		return s.find(search) != psl::string8::view::npos;
	}

	inline size_t contains(psl::string16::view s, psl::string16::view search) {
		return s.find(search) != psl::string16::view::npos;
	}

	inline size_t contains(psl::string32::view s, psl::string32::view search) {
		return s.find(search) != psl::string32::view::npos;
	}

	inline size_t count(psl::string8::view s, psl::string8::view search) {
		size_t occurrences					= 0u;
		psl::string8::view::size_type start = 0;

		while((start = s.find(search, start)) != psl::string8::view::npos) {
			++occurrences;
			start += search.length();
		}

		return occurrences;
	}

	inline size_t count(psl::string16::view s, psl::string16::view search) {
		size_t occurrences					 = 0u;
		psl::string16::view::size_type start = 0;

		while((start = s.find(search, start)) != psl::string16::view::npos) {
			++occurrences;
			start += search.length();
		}

		return occurrences;
	}

	inline size_t count(psl::string32::view s, psl::string32::view search) {
		size_t occurrences					 = 0u;
		psl::string32::view::size_type start = 0;

		while((start = s.find(search, start)) != psl::string32::view::npos) {
			++occurrences;
			start += search.length();
		}

		return occurrences;
	}


	inline bool replace(psl::string8_t& str, psl::string8::view from, psl::string8::view to) {
		size_t start_pos = str.find(from);
		if(start_pos == psl::string8_t::npos)
			return false;
		str.replace(start_pos, from.length(), to);
		return true;
	}

	inline bool replace(psl::string16_t& str, psl::string16::view from, psl::string16::view to) {
		size_t start_pos = str.find(from);
		if(start_pos == psl::string16_t::npos)
			return false;
		str.replace(start_pos, from.length(), to);
		return true;
	}
	inline bool replace(psl::string32_t& str, psl::string32::view from, psl::string32::view to) {
		size_t start_pos = str.find(from);
		if(start_pos == psl::string32_t::npos)
			return false;
		str.replace(start_pos, from.length(), to);
		return true;
	}

	inline bool replace_all(psl::string8_t& str, psl::string8::view from, psl::string8::view to) {
		if(from.empty())
			return false;
		size_t start_pos = 0;
		bool success	 = false;
		while((start_pos = str.find(from, start_pos)) != psl::string8_t::npos) {
			success = true;
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();	 // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
		return success;
	}

	inline psl::string8_t replace_all(psl::string8::view str, psl::string8::view from, psl::string8::view to) {
		psl::string8_t res {str};
		replace_all(res, from, to);
		return res;
	}

	inline bool replace_all(psl::string16_t& str, psl::string16::view from, psl::string16::view to) {
		if(from.empty())
			return false;
		size_t start_pos = 0;
		bool success	 = false;
		while((start_pos = str.find(from, start_pos)) != psl::string16_t::npos) {
			success = true;
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();	 // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
		return success;
	}

	inline psl::string16_t replace_all(psl::string16::view str, psl::string16::view from, psl::string16::view to) {
		psl::string16_t res {str};
		replace_all(res, from, to);
		return res;
	}

	inline bool replace_all(psl::string32_t& str, psl::string32::view from, psl::string32::view to) {
		if(from.empty())
			return false;
		size_t start_pos = 0;
		bool success	 = false;
		while((start_pos = str.find(from, start_pos)) != psl::string32_t::npos) {
			success = true;
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();	 // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
		return success;
	}

	inline psl::string32_t replace_all(psl::string32::view str, psl::string32::view from, psl::string32::view to) {
		psl::string32_t res {str};
		replace_all(res, from, to);
		return res;
	}

	inline size_t remove_all(psl::string8_t& str, psl::string8::view pattern) {
		size_t start_pos = 0;
		size_t count	 = 0;
		while((start_pos = str.find(pattern, start_pos)) != psl::string8::view::npos) {
			++count;
			str.erase(start_pos, pattern.size());
		}
		return count;
	}

	inline size_t remove_all(psl::string8_t& str, psl::string8::view pattern, size_t startOffset, size_t& endOffset) {
		size_t count	 = 0;
		size_t start_pos = startOffset;
		while((start_pos = str.find(pattern, start_pos)) != psl::string8_t::npos && start_pos < endOffset) {
			++count;
			str.erase(start_pos, pattern.size());
			endOffset -= pattern.size();
		}
		return count;
	}


	inline size_t remove_all(psl::string8_t& str,
							 const std::vector<psl::string8::view>& patterns,
							 size_t startOffset,
							 size_t& endOffset) {
		size_t count = 0;
		for(const auto& pattern : patterns) {
			count += remove_all(str, pattern, startOffset, endOffset);
		}
		return count;
	}

	inline size_t remove_all(psl::string16_t& str, psl::string16::view pattern) {
		size_t start_pos = 0;
		size_t count	 = 0;
		while((start_pos = str.find(pattern, start_pos)) != psl::string16::view::npos) {
			++count;
			str.erase(start_pos, pattern.size());
		}
		return count;
	}

	inline size_t remove_all(psl::string16_t& str, psl::string16::view pattern, size_t startOffset, size_t& endOffset) {
		size_t count	 = 0;
		size_t start_pos = startOffset;
		while((start_pos = str.find(pattern, start_pos)) != psl::string16_t::npos && start_pos < endOffset) {
			++count;
			str.erase(start_pos, pattern.size());
			endOffset -= pattern.size();
		}
		return count;
	}


	inline size_t remove_all(psl::string16_t& str,
							 const std::vector<psl::string16::view>& patterns,
							 size_t startOffset,
							 size_t& endOffset) {
		size_t count = 0;
		for(const auto& pattern : patterns) {
			count += remove_all(str, pattern, startOffset, endOffset);
		}
		return count;
	}


	inline size_t remove_all(psl::string32_t& str, psl::string32::view pattern) {
		size_t start_pos = 0;
		size_t count	 = 0;
		while((start_pos = str.find(pattern, start_pos)) != psl::string32::view::npos) {
			++count;
			str.erase(start_pos, pattern.size());
		}
		return count;
	}

	inline size_t remove_all(psl::string32_t& str, psl::string32::view pattern, size_t startOffset, size_t& endOffset) {
		size_t count	 = 0;
		size_t start_pos = startOffset;
		while((start_pos = str.find(pattern, start_pos)) != psl::string32_t::npos && start_pos < endOffset) {
			++count;
			str.erase(start_pos, pattern.size());
			endOffset -= pattern.size();
		}
		return count;
	}


	inline size_t remove_all(psl::string32_t& str,
							 const std::vector<psl::string32::view>& patterns,
							 size_t startOffset,
							 size_t& endOffset) {
		size_t count = 0;
		for(const auto& pattern : patterns) {
			count += remove_all(str, pattern, startOffset, endOffset);
		}
		return count;
	}


	inline std::vector<size_t> locations(psl::string8::view s, psl::string8::view search) {
		std::vector<size_t> occurenceLocations;
		psl::string8::view::size_type start = 0;

		while((start = s.find(search, start)) != psl::string8::view::npos) {
			occurenceLocations.push_back(start);
			start += search.length();
		}

		return occurenceLocations;
	}

	inline std::vector<size_t> locations(psl::string16::view s, psl::string16::view search) {
		std::vector<size_t> occurenceLocations;
		psl::string16::view::size_type start = 0;

		while((start = s.find(search, start)) != psl::string16::view::npos) {
			occurenceLocations.push_back(start);
			start += search.length();
		}

		return occurenceLocations;
	}

	inline std::vector<size_t> locations(psl::string32::view s, psl::string32::view search) {
		std::vector<size_t> occurenceLocations;
		psl::string32::view::size_type start = 0;

		while((start = s.find(search, start)) != psl::string32::view::npos) {
			occurenceLocations.push_back(start);
			start += search.length();
		}

		return occurenceLocations;
	}


	[[nodiscard]] inline psl::string_view::size_type
	rfind_first_of(psl::string_view source,
				   psl::string_view tokens,
				   const psl::string_view::size_type offset = psl::string_view::npos) {
		size_t i = std::min(offset, source.size());
		while(i != 0) {
			--i;
			if(std::find(std::begin(tokens), std::end(tokens), source[i]) != std::end(tokens))
				return i;
		}
		return psl::string_view::npos;
	}

	[[nodiscard]] inline psl::string_view::size_type
	rfind_first_of(psl::string_view source,
				   psl::char_t token,
				   const psl::string_view::size_type offset = psl::string_view::npos) {
		size_t i = std::min(offset, source.size());
		while(i != 0) {
			--i;
			if(token == source[i])
				return i;
		}
		return psl::string_view::npos;
	}


	[[nodiscard]] inline psl::string_view::size_type
	rfind_first_not_of(psl::string_view source,
					   psl::string_view tokens,
					   const psl::string_view::size_type offset = psl::string_view::npos) {
		size_t i = std::min(offset, source.size());

		while(i != 0) {
			--i;
			if(std::find(std::begin(tokens), std::end(tokens), source[i]) == std::end(tokens))
				return i + 1;
		}
		return psl::string_view::npos;
	}

	[[nodiscard]] inline psl::string_view::size_type
	rfind_first_not_of(psl::string_view source,
					   psl::char_t token,
					   const psl::string_view::size_type offset = psl::string_view::npos) {
		size_t i = std::min(offset, source.size());

		while(i != 0) {
			--i;
			if(token != source[i])
				return i + 1;
		}
		return psl::string_view::npos;
	}


	template <typename... Args>
	inline psl::string8_t format(const psl::string8_t& target, Args... args) {
		size_t size = snprintf(nullptr, 0, target.c_str(), args...) + 1;	// Extra space for '\0'
		std::unique_ptr<psl::string8::char_t[]> buf(new psl::string8::char_t[size]);
		snprintf(buf.get(), size, target.c_str(), args...);
		return psl::string8_t(buf.get(), buf.get() + size - 1);	   // We don't want the '\0' inside
	}
}	 // namespace string

namespace details {
	template <typename X, typename SFINEA = void>
	struct member_function_to_string : std::false_type {};

	template <typename X>
	struct member_function_to_string<X, std::void_t<decltype(std::declval<X&>().to_string())>> : std::true_type {};

	template <typename X, typename SFINEA = void>
	struct member_function_from_string : std::false_type {};

	template <typename X>
	struct member_function_from_string<
	  X,
	  std::void_t<decltype(std::declval<X&>().from_string(std::declval<psl::string8::view>()))>> : std::true_type {};

	template <typename T>
	concept HasStaticFromString = requires() {
		T::from_string(std::string_view {});
	};

	template <typename T>
	concept HasStdToString = requires(T t) {
		std::to_string(t);
	};
}	 // namespace details
template <typename X>
struct converter {
	template <typename Y = X>
	static typename std::enable_if_t<!std::is_enum<Y>::value, psl::string8_t> to_string(const X& x) {
		if constexpr(std::is_convertible<X, psl::string8_t>::value) {
			return x;
		} else if constexpr(details::member_function_to_string<X>::value) {
			return x.to_string();
		} else if constexpr(details::HasStdToString<X>) {
			return std::to_string(x);
		} else {
			static_assert(utility::templates::always_false_v<X>,
						  "no conversion possible from the given type, please write a converter specialization, or "
						  "a publically accessible to_string() method.");
		}
	}


	template <typename Y = X>
	static typename std::enable_if_t<std::is_enum<Y>::value, psl::string8_t> to_string(const X& x) {
		return converter<typename std::underlying_type<X>::type>::to_string(
		  static_cast<typename std::underlying_type<X>::type>(x));
	}

	template <typename Y = X>
	static typename std::enable_if_t<!std::is_enum<Y>::value, X> from_string(psl::string8::view str) {
		if constexpr(std::is_convertible<X, psl::string8_t>::value) {
			return {psl::string8_t(str)};
		} else if constexpr(details::HasStaticFromString<X>) {
			return X::from_string(str);
		} else {
			static_assert(utility::templates::always_false_v<X>,
						  "no conversion possible from the given type, please write a converter specialization, or "
						  "a publically accessible from_string(string_view) method.");
		}
	}

	template <typename Y = X>
	static typename std::enable_if_t<!std::is_enum<Y>::value, void> from_string(X& x, psl::string8::view str) {
		if constexpr(std::is_convertible<psl::string8_t, X>::value) {
			x = str;
		} else if constexpr(details::member_function_from_string<X>::value) {
			x.from_string(str);
		} else {
			static_assert(utility::templates::always_false_v<X>,
						  "no conversion possible from the given type, please write a converter specialization, or "
						  "a publically accessible from_string(string_view) method.");
		}
	}

	template <typename Y = X>
	static typename std::enable_if_t<std::is_enum<Y>::value, X> from_string(psl::string8::view str) {
		using enum_type = typename std::underlying_type<X>::type;
		return static_cast<X>(converter<enum_type>::from_string(str));
	}
};

template <>
struct converter<psl::string8_t> {
	using value_t	 = psl::string8_t;
	using view_t	 = psl::string8::view;
	using encoding_t = psl::string8_t;

	static encoding_t to_string(const value_t& x) { return x; }

	static value_t from_string(view_t str) { return value_t(str); }

	static void from_string(value_t& out, view_t str) { out = str; }

	static bool is_valid(view_t str) { return true; }
};

template <>
struct converter<psl::string16_t> {
	using value_t	 = psl::string16_t;
	using view_t	 = psl::string8::view;
	using encoding_t = psl::string8_t;

	static encoding_t to_string(const value_t& x) { return psl::string16::to_string8_t(x); }

	static value_t from_string(view_t str) { return psl::string16::from_string8_t(psl::string8_t {str}); }

	static void from_string(value_t& out, view_t str) { out = psl::string16::from_string8_t(psl::string8_t {str}); }

	static bool is_valid(view_t str) { return true; }
};

template <size_t N>
struct converter<char[N]> {
	using value_t	 = char[N];
	using view_t	 = psl::string8::view;
	using encoding_t = psl::string8_t;

	static encoding_t to_string(const value_t& x) { return x; }

	static encoding_t from_string(view_t str) { return encoding_t(str); }

	static void from_string(value_t& out, view_t str) { out = from_string(str); }

	static bool is_valid(view_t str) { return true; }
};
template <>
struct converter<bool> {
	static psl::string8_t to_string(const bool& x) { return std::to_string(x); }

	static bool from_string(psl::string8::view str) {
		psl::string8_t v {str};
		v = utility::string::to_lower(v);
		return v == "1" || v == "t" || v == "true";
	}

	static bool is_valid(psl::string8::view str) {
		psl::string8_t v = utility::string::to_lower(str);
		return v == "1" || v == "0" || v == "t" || v == "f" || v == "true" || v == "false";
	}
};

template <size_t N>
struct converter<std::bitset<N>> {
	using value_t	 = std::bitset<N>;
	using view_t	 = psl::string8::view;
	using encoding_t = psl::string8_t;

	static encoding_t to_string(const value_t& x) { return x.to_string(); }

	static value_t from_string(view_t str) { return value_t(encoding_t(str)); }

	static void from_string(value_t& out, view_t str) { out = from_string(str); }

	static bool is_valid(view_t str) { return str.find_first_not_of("0123456789") == view_t::npos; }
};


template <>
struct converter<float> {
	static psl::string8_t to_string(const float& x) { return std::to_string(x); }

	static float from_string(psl::string8::view str) { return std::stof(psl::string8_t(str)); }
};

template <>
struct converter<double> {
	static psl::string8_t to_string(const double& x) { return std::to_string(x); }

	static double from_string(psl::string8::view str) { return std::stod(psl::string8_t(str)); }
};

template <typename T>
requires(std::is_same_v<unsigned long, T> || std::is_same_v<uint64_t, T>) struct converter<T> {
	static psl::string8_t to_string(const T& x) { return std::to_string(x); }

	static T from_string(psl::string8::view str) { return std::stoull(psl::string8_t(str)); }
	static T from_string(T& target, psl::string8::view str) {
		target = std::stoull(psl::string8_t(str));
		return target;
	}
};
template <>
struct converter<uint32_t> {
	static psl::string8_t to_string(const uint32_t& x) { return std::to_string(x); }

	static uint32_t from_string(psl::string8::view str) { return std::stoul(psl::string8_t(str)); }
};
template <>
struct converter<uint16_t> {
	static psl::string8_t to_string(const uint16_t& x) { return std::to_string(x); }

	static uint16_t from_string(psl::string8::view str) { return (uint16_t)std::stoul(psl::string8_t(str)); }
};

template <>
struct converter<uint8_t> {
	static psl::string8_t to_string(const uint8_t& x) { return std::to_string(x); }

	static uint8_t from_string(psl::string8::view str) { return (uint8_t)std::stoul(psl::string8_t(str)); }
};

template <>
struct converter<int8_t> {
	static psl::string8_t to_string(const int8_t& x) { return std::to_string(x); }

	static int8_t from_string(psl::string8::view str) {
		int8_t x;
#if __has_include(<charconv>)
		std::from_chars(str.data(), str.data() + str.size(), x);
#else
		return std::stoi(psl::string8_t(str));
#endif
		return x;
	}
};

template <>
struct converter<int16_t> {
	static psl::string8_t to_string(const int16_t& x) { return std::to_string(x); }

	static int16_t from_string(psl::string8::view str) {
		int16_t x;
#if __has_include(<charconv>)
		std::from_chars(str.data(), str.data() + str.size(), x);
#else
		return std::stoi(psl::string8_t(str));
#endif
		return x;
	}
};

template <>
struct converter<int32_t> {
	static psl::string8_t to_string(const int32_t& x) { return std::to_string(x); }

	static int32_t from_string(psl::string8::view str) {
		int32_t x;
#if __has_include(<charconv>)
		std::from_chars(str.data(), str.data() + str.size(), x);
#else
		return std::stoi(psl::string8_t(str));
#endif
		return x;
	}
};
template <>
struct converter<int64_t> {
	static psl::string8_t to_string(const int64_t& x) { return std::to_string(x); }

	static int64_t from_string(psl::string8::view str) { return (int64_t)std::stoll(psl::string8_t(str)); }
};
// short hand version that calls the converter for you
template <typename T>
static T from_string(psl::string8::view str) {
#ifndef CONVERTER_NOEXCEPT
	try {
#endif
		return utility::converter<T>::from_string(str);
#ifndef CONVERTER_NOEXCEPT
	} catch(...) {
		return T {};
	}
#endif
}


template <typename T>
static bool from_string(psl::string8::view str, T& out) {
#ifndef CONVERTER_NOEXCEPT
	try {
#endif
		utility::converter<T>::from_string(out, str);
		return true;
#ifndef CONVERTER_NOEXCEPT
	} catch(...) {
		return false;
	}
#endif
}

template <typename T>
static psl::string8_t to_string(const T& target) {
#ifndef CONVERTER_NOEXCEPT
	try {
#endif
		return utility::converter<T>::to_string(target);
#ifndef CONVERTER_NOEXCEPT
	} catch(...) {
		return {};
	}
#endif
}
}	 // namespace utility
