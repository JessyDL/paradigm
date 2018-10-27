#pragma once
#include <cstdint>

/// \brief compile-time uint8_t literal operator.
/// \param[in] value the value to convert to uint8_t.
/// \returns the uint8_t value based on the input value.
constexpr std::uint8_t operator "" _ui8(unsigned long long value)
{
	return static_cast<std::uint8_t>(value);
}

/// \brief compile-time uint16_t literal operator.
/// \param[in] value the value to convert to uint16_t.
/// \returns the uint16_t value based on the input value.
constexpr std::uint16_t operator "" _ui16(unsigned long long value)
{
	return static_cast<std::uint16_t>(value);
}

/// \brief compile-time uint32_t literal operator.
/// \param[in] value the value to convert to uint32_t.
/// \returns the uint32_t value based on the input value.
constexpr std::uint32_t operator "" _ui32(unsigned long long value)
{
	return static_cast<std::uint32_t>(value);
}

/// \brief compile-time uint64_t literal operator.
/// \param[in] value the value to convert to uint64_t.
/// \returns the uint64_t value based on the input value.
constexpr std::uint64_t operator "" _ui64(unsigned long long value)
{
	return static_cast<std::uint64_t>(value);
}

/// \brief compile-time degree to radians literal operator.
/// \param[in] deg the value to convert to radians.
/// \returns the radians value based on the input value.
constexpr long double operator"" _deg_to_rad(long double deg)
{
	return deg * 3.141592 / 180;
}
