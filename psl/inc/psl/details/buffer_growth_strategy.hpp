#pragma once
#include <algorithm>
#include <cstddef>

namespace psl::details {
struct default_buffer_growth_strategy_t {
	size_t growth(size_t current, size_t min) const noexcept {
		return std::max<size_t>(current, min + (min >> 1));
	}
};
}	 // namespace psl::details
