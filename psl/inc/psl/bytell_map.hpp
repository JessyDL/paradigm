#pragma once
#define DISABLE_EXTENSIONS
#ifndef DISABLE_EXTENSIONS
#include "psl/bytell_hash_map.hpp"

namespace psl
{
	template <typename KT, typename VT>
	using bytell_map = ska::bytell_hash_map<KT, VT>;
}

#else
#include <unordered_map>
namespace psl
{
	template <typename KT, typename VT>
	using bytell_map = std::unordered_map<KT, VT>;
}
#endif