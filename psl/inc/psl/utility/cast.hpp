#pragma once
#include "psl/assertions.hpp"

namespace psl::utility
{
	template<typename TargetType>
	constexpr auto narrow_cast(auto v) noexcept -> TargetType
	{
		psl_assert(static_cast<TargetType>(v) == v, "narrow_cast<>() failed");
		return static_cast<TargetType>(v);
	}
}