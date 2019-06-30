#pragma once
#include "component_key.h"
#include "array.h"
#include "pack_view.h"


namespace psl::ecs::details
{
	struct entity_info
	{
		entity_info() noexcept = default;
		~entity_info() = default;
		entity_info(const entity_info&) = default;
		entity_info(entity_info&&) noexcept = default;
		entity_info& operator=(const entity_info&) = default;
		entity_info& operator=(entity_info&&) = default;

		inline void emplace_back(component_key_t key, size_t indice)
		{
			m_Components.emplace_back(key);
			m_Indices.emplace_back(indice);
		}
		inline void emplace_back(std::pair<component_key_t, size_t> pair)
		{
			m_Components.emplace_back(pair.first);
			m_Indices.emplace_back(pair.second);
		}

		inline void erase(component_key_t key, size_t likely_index)
		{
			if(m_Components.size() > likely_index && m_Components[likely_index] == key)
			{
				m_Components.erase(std::next(std::begin(m_Components), likely_index));
				m_Indices.erase(std::next(std::begin(m_Indices), likely_index));
			}
			else
			{
				erase(key);
			}
		}

		inline void erase(component_key_t key)
		{
			auto it	= std::find(std::begin(m_Components), std::end(m_Components), key);
			auto index = std::distance(std::begin(m_Components), it);
			m_Components.erase(it);
			m_Indices.erase(std::next(std::begin(m_Indices), index));
		}

		inline void clear()
		{
			m_Components.clear();
			m_Indices.clear();
		}

		inline size_t unsafe_index(component_key_t key) const noexcept
		{
			auto it = std::find(m_Components.begin(), m_Components.end(), key);
			return m_Indices[std::distance(std::begin(m_Components), it)];
		}

		inline bool index(component_key_t key, size_t& out) const noexcept
		{
			auto it = std::find(m_Components.begin(), m_Components.end(), key);
			if(it == std::end(m_Components)) return false;
			out = m_Indices[std::distance(std::begin(m_Components), it)];
			return true;
		}

		inline psl::pack_view<component_key_t, size_t> zip() const noexcept
		{
			return psl::zip(psl::array_view<component_key_t>{m_Components}, psl::array_view<size_t>{m_Indices});
		}

		inline bool has(component_key_t key) const noexcept
		{
			return std::find(std::begin(m_Components), std::end(m_Components), key) != std::end(m_Components);
		}

		inline bool orphan() const noexcept { return m_Indices.size() == 0; }

		inline psl::array_view<component_key_t> components() const noexcept { return m_Components; }

		inline void append(entity_info&& info)
		{
			m_Components.insert(std::end(m_Components), std::begin(info.m_Components), std::end(info.m_Components));
			m_Indices.insert(std::end(m_Indices), std::begin(info.m_Indices), std::end(info.m_Indices));
		}
	  private:
		psl::array<component_key_t> m_Components;
		psl::array<size_t> m_Indices;
	};
} // namespace psl::ecs::details