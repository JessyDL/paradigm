#pragma once
#if INSTRUCTION_SET == 1
#include "psl/math/matrix.hpp"
#include <xmmintrin.h>

namespace psl
{
	template <typename precision_t, size_t d1, size_t d2, size_t d3>
	constexpr tmat<precision_t, d1, d3> operator*(const tmat<precision_t, d1, d2>& left,
												  const tmat<precision_t, d2, d3>& right) noexcept
	{
		tmat<precision_t, d1, d3> res{1};
		if constexpr(d1 * d3 == 16)
		{
			__m128 a_line, b_line, r_line;
			for(int i = 0; i < 16; i += 4)
			{
				// unroll the first step of the loop to avoid having to initialize r_line to zero
				a_line = _mm_load_ps(left.value.data());         // a_line = vec4(column(a, 0))
				b_line = _mm_set1_ps(right[i]);      // b_line = vec4(b[i][0])
				r_line = _mm_mul_ps(a_line, b_line); // r_line = a_line * b_line
				for(int j = 1; j < 4; j++)
				{
					a_line = _mm_load_ps(&left[j * 4]); // a_line = vec4(column(a, j))
					b_line = _mm_set1_ps(right[i + j]);  // b_line = vec4(b[i][j])
												   // r_line += a_line * b_line
					r_line = _mm_add_ps(_mm_mul_ps(a_line, b_line), r_line);
				}
				_mm_store_ps(&res[i], r_line);     // r[i] = r_line
			}
		}
		else
		{
			for(size_t column = 0; column < d1; column++)
			{
				for(size_t row = 0; row < d3; row++)
				{
					res[{row, column}] = precision_t{0};
					for(size_t p = 0; p < d2; p++)
					{
						res[{row, column}] += left[{p, column}] * right[{row, p}];
					}
				}
			}
		}
		return res;
	}
}
#endif