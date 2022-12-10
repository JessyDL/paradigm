#pragma once
#include <cstdint>
#include <type_traits>

namespace psl::ecs::details
{
	/// \brief Defines the discrete stages the `staged_sparse_memory_region_t` can store
	enum class stage_t : uint8_t
	{
		SETTLED = 0,	// values that have persisted for one promotion, and aren't about to be removed
		ADDED	= 1,	// values that have just been added
		REMOVED = 2,	// values slated for removal with the next promote calld values
	};

	/// \brief Defines types of ranges you can safely interact with in a contiguous way
	enum class stage_range_t : uint8_t
	{
		SETTLED,	 // values that have persisted for one promotion, and aren't about to be removed
		ADDED,		 // values that have just been added
		REMOVED,	 // values slated for removal with the next promote call
		ALIVE,		 // all non-removed values
		TERMINAL,	 // values that have just been added, and to-be removed values
		ALL,		 // all values
	};

	
	template <typename T>
	constexpr FORCEINLINE auto to_underlying(T value) noexcept -> std::underlying_type_t<T>
	{
		return static_cast<std::underlying_type_t<T>>(value);
	}

	constexpr FORCEINLINE auto stage_begin(stage_range_t stage) noexcept -> size_t
	{
		switch(stage)
		{
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
		psl::unreachable("stage was of unknown value");
	}

	constexpr FORCEINLINE auto stage_end(stage_range_t stage) noexcept -> size_t
	{
		switch(stage)
		{
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
		psl::unreachable("stage was of unknown value");
	}
}
