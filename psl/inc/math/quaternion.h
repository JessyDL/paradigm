#pragma once
#include "vec.h"

namespace psl
{
	template<typename precision_t>
	using tquat = tvec<precision_t, 4>;

	using quat = tquat<float>;
	using dquat = tquat<double>;
	using iquat = tquat<int>;
	using quat_sz = tquat<size_t>;
}