#pragma once
#include "memory/raw_region.h"
#include "entity.h"
#include <vector>
#include "component_key.h"
#include "IDGenerator.h"

namespace core::ecs
{
	template <typename T>
	class empty
	{};
}
namespace core::ecs::details
{
	struct component_info
	{
		component_info() = default;
		component_info(memory::raw_region&& region, std::vector<entity>&& entities, component_key_t id, size_t size)
			: region(std::move(region)), entities(std::move(entities)), id(id), size(size),
			generator(region.size() / size)
		{};

		component_info(std::vector<entity>&& entities, component_key_t id)
			: region(128), entities(std::move(entities)), id(id), size(1), generator(1)
		{};
		memory::raw_region region;
		std::vector<entity> entities;
		component_key_t id;
		size_t size;
		psl::IDGenerator<uint64_t> generator;
	};
}