#pragma once
#include "component_key.h"
#include "vector.h"
#include "pack_view.h"


namespace psl::ecs::details
{
	struct entity_info
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

		psl::array<component_key_t> components;
		psl::array<size_t> indices;
	};
}