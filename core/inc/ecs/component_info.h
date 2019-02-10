#pragma once
#include "memory/raw_region.h"
#include "entity.h"
#include <vector>
#include "component_key.h"
#include "IDGenerator.h"
#include "pack_view.h"

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
		~component_info() = default;
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
		std::vector<entity> removed_entities;
		memory::raw_region region;
		psl::generator<uint64_t> generator;
		component_key_t id;
		size_t capacity;
		size_t size;
	};

	struct entity_data
	{
		void emplace_back(component_key_t key, size_t indice)
		{
			components.emplace_back(key);
			indices.emplace_back(indice);
		}
		void emplace_back(std::pair<component_key_t, size_t> pair)
		{
			components.emplace_back(pair.first);
			indices.emplace_back(pair.second);
		}

		void erase(size_t index) 
		{
			components.erase(std::next(std::begin(components), index));
			indices.erase(std::next(std::begin(indices), index));
		}

		void erase(component_key_t key)
		{
			auto it = std::find(std::begin(components), std::end(components), key);
			auto index = std::distance(std::begin(components), it);
			components.erase(it);
			indices.erase(std::next(std::begin(indices), index));
		}

		size_t unsafe_index(component_key_t key) const
		{
			auto it = std::find(components.begin(), components.end(), key);
			return indices[std::distance(std::begin(components), it)];
		}

		bool index(component_key_t key, size_t& out) const
		{
			auto it = std::find(components.begin(), components.end(), key);
			if(it == std::end(components))
				return false;
			out = indices[std::distance(std::begin(components), it)];
			return true;
		}

		psl::pack_view<component_key_t, size_t> zip() const noexcept
		{
			return psl::zip(psl::array_view< component_key_t>{components}, psl::array_view< size_t>{indices});
		}
		std::vector<component_key_t> components;
		std::vector<size_t> indices;
	};

} // namespace core::ecs::details