#pragma once
#include "vec.hpp"

namespace psl::math
{
template <typename precision_t = double>
class constants
{
  public:
	static constexpr precision_t PI {static_cast<precision_t>(
	  3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798214808651328230664709384460955058223172535940812848111745028410270)};
	static constexpr precision_t DEG {precision_t {180} / PI};
	static constexpr precision_t RAD {PI / precision_t {180}};
};

template <typename T>
static constexpr T radians(const T& value) noexcept
{
	static_assert(std::numeric_limits<T>::is_iec559,
				  "the input type has to be true for std::numeric_limits<T>::is_iec559");

	return value * constants<T>::RAD;
}

template <typename T, size_t dimensions>
static constexpr T radians(const tvec<T, dimensions>& value) noexcept
{
	static_assert(std::numeric_limits<T>::is_iec559,
				  "the input type has to be true for std::numeric_limits<T>::is_iec559");

	return value * tvec<T, dimensions>(constants<T>::RAD);
}


template <typename T>
static constexpr T degrees(const T& value) noexcept
{
	static_assert(std::numeric_limits<T>::is_iec559,
				  "the input type has to be true for std::numeric_limits<T>::is_iec559");

	return value * constants<T>::DEG;
}

template <typename T, size_t dimensions>
static constexpr T degrees(const tvec<T, dimensions>& value) noexcept
{
	static_assert(std::numeric_limits<T>::is_iec559,
				  "the input type has to be true for std::numeric_limits<T>::is_iec559");

	return value * tvec<T, dimensions>(constants<T>::DEG);
}

template <typename precision_t>
static constexpr bool compare_to(const precision_t& left, const precision_t& right, uint8_t decimals) noexcept
{
	int64_t pow = (int64_t)std::pow(10, decimals);
	return (int64_t)(left * pow) == (int64_t)(right * pow);
}
}	 // namespace psl::math