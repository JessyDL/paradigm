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
} // namespace core::ecs
namespace core::ecs::details
{
	struct component_info
	{
		component_info() = default;
		component_info(component_key_t id, size_t size, size_t capacity = 256)
			: entities(), region(capacity * size), generator(std::numeric_limits<uint64_t>::max()), id(id),
			  capacity(capacity), size(size)
		{}
		component_info(component_key_t id) : entities(), region(1), generator(1), id(id), capacity(1), size(0){};

		component_info(const component_info& other)
			: entities(other.entities), region(other.region), generator(other.generator), id(other.id),
			  capacity(other.capacity), size(other.size){};
		component_info(component_info&& other)
			: entities(std::move(other.entities)), region(std::move(other.region)), generator(std::move(other.generator)), id(std::move(other.id)),
			capacity(std::move(other.capacity)), size(std::move(other.size))
		{};
		component_info& operator=(const component_info& other)
		{
			if(this != &other)
			{
				entities = other.entities;
				region = other.region;
				generator = other.generator;
				id = other.id;
				capacity = other.capacity;
				size = other.size;
			}
			return *this;
		}
		component_info& operator=(component_info&& other)
		{
			if(this != &other)
			{
				entities = std::move(other.entities);
				region = std::move(other.region);
				generator = std::move(other.generator);
				id = std::move(other.id);
				capacity = std::move(other.capacity);
				size = std::move(other.size);
			}
			return *this;
		}
		void grow()
		{
			if(is_tag()) return;

			memory::raw_region new_region{capacity * 2 * size};
			std::memcpy(new_region.data(), region.data(), capacity * size);
			capacity *= 2;
			region = std::move(new_region);
		}

		void shrink()
		{
			if(is_tag()) return;
		}

		void resize(size_t new_capacity)
		{
			if(is_tag()) return;

			memory::raw_region new_region{new_capacity * size};
			std::memcpy(new_region.data(), region.data(), capacity * size);
			capacity = new_capacity;
			region   = std::move(new_region);
		}

		uint64_t available() const noexcept { return capacity - entities.size(); }

		bool is_tag() const noexcept { return size == 0; }

		std::vector<entity> entities;
		memory::raw_region region;
		psl::generator<uint64_t> generator;
		component_key_t id;
		size_t capacity;
		size_t size;
	};
} // namespace core::ecs::details