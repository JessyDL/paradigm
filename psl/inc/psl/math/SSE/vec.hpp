#pragma once
#if INSTRUCTION_SET == 1
#include "psl/math/vec.hpp"
#include <xmmintrin.h>
namespace psl
{
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator+=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_add_ps(
							 _mm_load_ps(owner.value.data()),
							 _mm_load_ps(other.value.data())
						 )
			);
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_add_pd(
							 _mm_load_pd(owner.value.data()),
							 _mm_load_pd(other.value.data())
						 )
			);
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
	constexpr tvec<precision_t, 4>& operator*=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_mul_ps(
							 _mm_load_ps(owner.value.data()),
							 _mm_load_ps(other.value.data())
						 )
			);
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_mul_pd(
							 _mm_load_pd(owner.value.data()),
							 _mm_load_pd(other.value.data())
						 )
			);
		}
		else
		{
			owner.value[0] *= other.value[0];
			owner.value[1] *= other.value[1];
			owner.value[2] *= other.value[2];
			owner.value[3] *= other.value[3];
		}
		return owner;
	}


	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator/=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
	#ifdef MATH_DIV_ZERO_CHECK
		if(other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0 || other.value[3] == 0)
			throw std::runtime_exception("division by 0");
	#endif
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_div_ps(
							 _mm_load_ps(owner.value.data()),
							 _mm_load_ps(other.value.data())
						 )
			);
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_div_pd(
							 _mm_load_pd(owner.value.data()),
							 _mm_load_pd(other.value.data())
						 )
			);
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
	constexpr tvec<precision_t, 4>& operator-=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_sub_ps(
							 _mm_load_ps(owner.value.data()),
							 _mm_load_ps(other.value.data())
						 )
			);
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_sub_pd(
							 _mm_load_pd(owner.value.data()),
							 _mm_load_pd(other.value.data())
						 )
			);
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