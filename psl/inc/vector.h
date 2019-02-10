#pragma once
#include <vector>

namespace psl
{
	template<typename T>
	using vector = std::vector<T>;

	template<typename T>
	using array = psl::vector<T>;
}