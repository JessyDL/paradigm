#pragma once
#if INSTRUCTION_SET == 0
#include "psl/math/matrix.hpp"

namespace psl
{
	template <typename precision_t, size_t d1, size_t d2, size_t d3>
	constexpr tmat<precision_t, d1, d3> operator*(const tmat<precision_t, d1, d2>& left,
												  const tmat<precision_t, d2, d3>& right) noexcept
	{
		tmat<precision_t, d1, d3> res {1};
		for(size_t column = 0; column < d1; column++)
		{
			for(size_t row = 0; row < d3; row++)
			{
				res[{row, column}] = precision_t {0};
				for(size_t p = 0; p < d2; p++)
				{
					res[{row, column}] += left[{p, column}] * right[{row, p}];
				}
			}
		}
		return res;
	}
}	 // namespace psl
#endif