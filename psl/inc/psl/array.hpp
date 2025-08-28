#pragma once
#include <vector>

namespace psl {
template <typename T>
using vector = std::vector<T>;

template <typename T>
using array = psl::vector<T>;

template <typename T>
concept IsRangeType = requires(T t) {
// todo std::convertible_to is not available in android ndk
#if !defined(PE_PLATFORM_ANDROID)
	{ t.begin() } -> std::same_as<typename T::iterator>;
	{ t.end() } -> std::same_as<typename T::iterator>;
	{ t.size() } -> std::convertible_to<size_t>;
#else
	{ t.begin() };
	{ t.end() };
	{ t.size() };
#endif
};
}	 // namespace psl
