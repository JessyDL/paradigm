#pragma once
#include "vec.h"

namespace psl::math
{
	static constexpr double PI
	{
		3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798214808651328230664709384460955058223172535940812848111745028410270
	};

	static constexpr double DEG{ 180.0 / PI };
	static constexpr double RAD{ PI / 180.0 };

	template<typename T>
	static constexpr T radians(const T& value) noexcept
	{
		static_assert(std::numeric_limits<T>::is_iec559, "the input type has to be true for std::numeric_limits<T>::is_iec559");

		return value * static_cast<T>(RAD);
	}

	template<typename T, size_t dimensions>
	static constexpr T radians(const tvec<T, dimensions>& value) noexcept
	{
		static_assert(std::numeric_limits<T>::is_iec559, "the input type has to be true for std::numeric_limits<T>::is_iec559");

		return value * tvec<T, dimensions>(static_cast<T>(RAD));
	}


	template<typename T>
	static constexpr T degrees(const T& value) noexcept
	{
		static_assert(std::numeric_limits<T>::is_iec559, "the input type has to be true for std::numeric_limits<T>::is_iec559");

		return value * static_cast<T>(DEG);
	}

	template<typename T, size_t dimensions>
	static constexpr T degrees(const tvec<T, dimensions>& value) noexcept
	{
		static_assert(std::numeric_limits<T>::is_iec559, "the input type has to be true for std::numeric_limits<T>::is_iec559");

		return value * tvec<T, dimensions>(static_cast<T>(DEG));
	}
}