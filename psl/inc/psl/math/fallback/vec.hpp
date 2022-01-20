#pragma once
#if INSTRUCTION_SET == 0
#include "psl/math/vec.hpp"

namespace psl
{
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator+=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		owner.value[0] += other.value[0];
		owner.value[1] += other.value[1];
		owner.value[2] += other.value[2];
		owner.value[3] += other.value[3];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator*=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		owner.value[0] *= other.value[0];
		owner.value[1] *= other.value[1];
		owner.value[2] *= other.value[2];
		owner.value[3] *= other.value[3];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator/=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		owner.value[0] /= other.value[0];
		owner.value[1] /= other.value[1];
		owner.value[2] /= other.value[2];
		owner.value[3] /= other.value[3];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator-=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		owner.value[0] -= other.value[0];
		owner.value[1] -= other.value[1];
		owner.value[2] -= other.value[2];
		owner.value[3] -= other.value[3];
		return owner;
	}
}	 // namespace psl
#endif