#pragma once
#include "types.h"
#include "psl/static_array.h"
#include <numeric>

namespace core::gfx
{
	struct limits
	{
		uint64_t storage_buffer_offset_alignment;
		uint64_t uniform_buffer_offset_alignment;
		uint64_t minMemoryMapAlignment;
		core::gfx::format supported_depthformat;

		std::array<uint32_t, 3> compute_worgroup_size;
		std::array<uint32_t, 3> compute_worgroup_count;
		uint32_t compute_worgroup_invocations;
	};

	constexpr inline limits min(const limits& l, const limits& r) noexcept
	{
		limits limit{};

		limit.storage_buffer_offset_alignment =
			std::lcm(l.storage_buffer_offset_alignment, r.storage_buffer_offset_alignment);

		limit.uniform_buffer_offset_alignment =
			std::lcm(l.uniform_buffer_offset_alignment, r.uniform_buffer_offset_alignment);

		limit.minMemoryMapAlignment =
			std::lcm(l.minMemoryMapAlignment, r.minMemoryMapAlignment);

		for(int i = 0; i < l.compute_worgroup_size.size(); ++i)
		{
			limit.compute_worgroup_size[i]  = std::min(l.compute_worgroup_size[i], r.compute_worgroup_size[i]);
			limit.compute_worgroup_count[i] = std::min(l.compute_worgroup_count[i], r.compute_worgroup_count[i]);
		}
		limit.compute_worgroup_invocations = std::min(l.compute_worgroup_invocations, r.compute_worgroup_invocations);

		return limit;
	}
} // namespace core::gfx