#pragma once
#include <unordered_map>

namespace psl
{
	template<typename KT, typename VT>
	using unordered_map = std::unordered_map<KT, VT>;
}