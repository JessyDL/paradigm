#pragma once
#include "psl/static_array.hpp"
#include "types.hpp"
#include <numeric>

namespace core::gfx {
struct limits {
	struct buffer {
		uint64_t alignment;
		uint64_t size;
	};

	buffer uniform;
	buffer storage;
	buffer memorymap;

	struct {
		struct {
			std::array<uint32_t, 3> size;
			std::array<uint32_t, 3> count;
			uint32_t invocations;
		} workgroup;
	} compute;

	core::gfx::format_t supported_depthformat;
};

constexpr inline limits::buffer min(const limits::buffer& l, const limits::buffer& r) noexcept {
	limits::buffer limit {};
	limit.alignment = std::lcm(l.alignment, r.alignment);
	limit.size		= std::min(l.size, r.size);
	return limit;
};

constexpr inline limits min(const limits& l, const limits& r) noexcept {
	limits limit {};

	limit.storage	= min(l.storage, r.storage);
	limit.uniform	= min(l.uniform, r.uniform);
	limit.memorymap = min(l.memorymap, r.memorymap);

	for(int i = 0; i < l.compute.workgroup.size.size(); ++i) {
		limit.compute.workgroup.size[i]	 = std::min(l.compute.workgroup.size[i], r.compute.workgroup.size[i]);
		limit.compute.workgroup.count[i] = std::min(l.compute.workgroup.count[i], r.compute.workgroup.count[i]);
	}
	limit.compute.workgroup.invocations = std::min(l.compute.workgroup.invocations, r.compute.workgroup.invocations);

	return limit;
}
}	 // namespace core::gfx
