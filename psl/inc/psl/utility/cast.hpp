#pragma once
#include "psl/assertions.hpp"

namespace psl::utility {
/// \brief Checks if the conversion would result in an overflow when asserts are enabled
/// \tparam TargetType Type to convert to
/// \param v Value to convert
/// \returns The result of the conversion
template <typename TargetType>
constexpr auto narrow_cast(auto v) noexcept -> TargetType {
	psl_assert(static_cast<TargetType>(v) == v, "narrow_cast<>() failed");
	return static_cast<TargetType>(v);
}
}	 // namespace psl::utility
