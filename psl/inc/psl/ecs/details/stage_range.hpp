#pragma once
#include "psl/utility/enum.hpp"
#include <cstdint>
#include <type_traits>

namespace psl::ecs::details {
/// \brief Defines the discrete stages the `staged_sparse_memory_region_t` can store
enum class stage_t : uint8_t {
	SETTLED = 0,	// values that have persisted for one promotion, and aren't about to be removed
	ADDED	= 1,	// values that have just been added
	REMOVED = 2,	// values slated for removal with the next promote calld values
};
}	 // namespace psl::ecs::details

template <>
inline constexpr psl::utility::enum_ops_t psl::utility::enable_enum_ops<psl::ecs::details::stage_t> =
  psl::utility::enum_ops_t::BIT;

namespace psl::ecs::details {
/// \brief Defines types of ranges you can safely interact with in a contiguous way
enum class stage_range_t : uint8_t {
	SETTLED = 1 << to_underlying(stage_t::SETTLED),	   // values that have persisted for one promotion, and aren't about
													   // to be removed
	ADDED	= 1 << to_underlying(stage_t::ADDED),	   // values that have just been added
	REMOVED = 1 << to_underlying(stage_t::REMOVED),	   // values slated for removal with the next promote call

	ALIVE	  = SETTLED | ADDED,			  // all non-removed values
	TERMINAL  = ADDED | REMOVED,			  // values that have just been added, and to-be removed values
	NOT_ADDED = SETTLED | REMOVED,			  // all non-added values
	ALL		  = SETTLED | ADDED | REMOVED,	  // all values
};
}	 // namespace psl::ecs::details

template <>
inline constexpr psl::utility::enum_ops_t psl::utility::enable_enum_ops<psl::ecs::details::stage_range_t> =
  psl::utility::enum_ops_t::BIT;

namespace psl::ecs::details {
constexpr FORCEINLINE auto stage_begin(stage_range_t stage) noexcept -> size_t {
	psl_assert(stage != stage_range_t::NOT_ADDED, "cannot do split ranges");
	switch(stage) {
	case stage_range_t::SETTLED:
	case stage_range_t::ALIVE:
	case stage_range_t::ALL:
		return to_underlying(stage_t::SETTLED);
	case stage_range_t::ADDED:
	case stage_range_t::TERMINAL:
		return to_underlying(stage_t::ADDED);
	case stage_range_t::REMOVED:
		return to_underlying(stage_t::REMOVED);
	}
	psl::unreachable("stage was of unknown or invalid value");
}

constexpr FORCEINLINE auto stage_end(stage_range_t stage) noexcept -> size_t {
	psl_assert(stage != stage_range_t::NOT_ADDED, "cannot do split ranges");
	switch(stage) {
	case stage_range_t::SETTLED:
		return to_underlying(stage_t::ADDED);
	case stage_range_t::ALIVE:
	case stage_range_t::ADDED:
		return to_underlying(stage_t::REMOVED);
	case stage_range_t::TERMINAL:
	case stage_range_t::REMOVED:
	case stage_range_t::ALL:
		return to_underlying(stage_t::REMOVED) + 1;
	}
	psl::unreachable("stage was of unknown or invalid value");
}
}	 // namespace psl::ecs::details
