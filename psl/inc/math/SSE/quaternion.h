#pragma once
#if INSTRUCTION_SET == 1
#include "math/quaternion.h"
namespace psl
{
	template <typename precision_t>
	tquat<precision_t>& operator+=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept
	{
		if constexpr(std::is_same<float, precision_t>::value)
		{
			auto ownL{_mm_load_ps(owner.value.data())};
			auto othL{_mm_load_ps(other.value.data())};
			auto addR{_mm_add_ps(ownL, othL)};
			_mm_store_ps(owner.value.data(), addR);
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_add_pd(_mm_load_pd(owner.value.data()), _mm_load_pd(other.value.data())));
		}
		else
		{
			owner.value[0] += other.value[0];
			owner.value[1] += other.value[1];
			owner.value[2] += other.value[2];
			owner.value[3] += other.value[3];
		}
		return owner;
	}

	template <typename precision_t>
	constexpr tquat<precision_t>& operator*=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept
	{
		tquat<precision_t> left = owner;
		owner.value[0] = left.value[0] * other.value[3] + left.value[1] * other.value[2] - left.value[2] * other.value[1] + left.value[3] * other.value[0];
		owner.value[1] = -left.value[0] * other.value[2] + left.value[1] * other.value[3] + left.value[2] * other.value[0] + left.value[3] * other.value[1];
		owner.value[2] = left.value[0] * other.value[1] - left.value[1] * other.value[0] + left.value[2] * other.value[3] + left.value[3] * other.value[2];
		owner.value[3] = -left.value[0] * other.value[0] - left.value[1] * other.value[1] - left.value[2] * other.value[2] + left.value[3] * other.value[3];
		return owner;
	}


	template <typename precision_t>
	constexpr tquat<precision_t>& operator/=(tquat<precision_t>& owner, const tquat<precision_t>& other)
	{
	#ifdef MATH_DIV_ZERO_CHECK
		if(other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0 || other.value[3] == 0)
			throw std::runtime_exception("division by 0");
	#endif
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_div_ps(_mm_load_ps(owner.value.data()), _mm_load_ps(other.value.data())));
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_div_pd(_mm_load_pd(owner.value.data()), _mm_load_pd(other.value.data())));
		}
		else
		{
			owner.value[0] /= other.value[0];
			owner.value[1] /= other.value[1];
			owner.value[2] /= other.value[2];
			owner.value[3] /= other.value[3];
		}
		return owner;
	}
	template <typename precision_t>
	constexpr tquat<precision_t>& operator-=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept
	{
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_sub_ps(_mm_load_ps(owner.value.data()), _mm_load_ps(other.value.data())));
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_sub_pd(_mm_load_pd(owner.value.data()), _mm_load_pd(other.value.data())));
		}
		else
		{
			owner.value[0] -= other.value[0];
			owner.value[1] -= other.value[1];
			owner.value[2] -= other.value[2];
			owner.value[3] -= other.value[3];
		}
		return owner;
	}
}

#endif