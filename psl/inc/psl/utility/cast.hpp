#pragma once
#include "psl/assertions.hpp"

namespace psl {
/// \brief Checks if the conversion would result in an overflow when asserts are enabled
/// \tparam TargetType Type to convert to
/// \param v Value to convert
/// \returns The result of the conversion
template <typename TargetType>
constexpr auto narrow_cast(auto v) noexcept -> TargetType {
	psl_assert(static_cast<TargetType>(v) == v, "narrow_cast<>() failed");
	return static_cast<TargetType>(v);
}

/// note this is missing on the android implementation
/// @brief converts an enum class to its underlying type
/// @tparam T
/// @param value enum to cast to underlying value
/// @return the value of the enum casted to its underlying type
/// @todo when android supports this remove
template <typename T>
constexpr FORCEINLINE auto to_underlying(T value) noexcept -> std::underlying_type_t<T> {
	return static_cast<std::underlying_type_t<T>>(value);
}
}	 // namespace psl
