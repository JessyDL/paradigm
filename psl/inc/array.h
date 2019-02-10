#pragma once
#include <array>

namespace psl
{
	template<typename T, size_t N = 0>
	using static_array = std::array<T, N>;
}