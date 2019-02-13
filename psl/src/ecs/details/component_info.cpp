#include "ecs/details/component_info.h"
#include "ecs/entity.h"
#include "memory/raw_region.h"
#include <algorithm>
#include "pack_view.h"
#include <numeric>

using namespace psl::ecs;
using namespace psl::ecs::details;

component_info::component_info(component_key_t id, size_t size, size_t capacity)
	: m_Entities(), m_Region((size > 0) ? new memory::raw_region(capacity * size) : nullptr), m_ID(id),
	  m_Capacity((size > 0) ? capacity : 0), m_Size(size)
{}

component_info::component_info(component_info&& other)
	: m_Entities(std::move(other.m_Entities)), m_Region(std::move(other.m_Region)), m_ID(std::move(other.m_ID)),
	  m_Capacity(std::move(other.m_Capacity)), m_Size(std::move(other.m_Size))
{
	other.m_Region = nullptr;
};

component_info::~component_info()
{
	if(m_Region != nullptr) delete(m_Region);
}

component_info& component_info::operator=(component_info&& other)
{
	if(this != &other)
	{
		m_Entities	 = std::move(other.m_Entities);
		m_Region	   = std::move(other.m_Region);
		m_ID		   = std::move(other.m_ID);
		m_Capacity	 = std::move(other.m_Capacity);
		m_Size		   = std::move(other.m_Size);
		other.m_Region = nullptr;
	}
	return *this;
}
void component_info::grow()
{
	if(is_tag()) return;

	memory::raw_region* new_region{new memory::raw_region(m_Capacity * 2 * m_Size)};
	std::memcpy(new_region->data(), m_Region->data(), m_Capacity * m_Size);
	m_Capacity *= 2;
	delete(m_Region);
	m_Region = new_region;
}

void component_info::shrink()
{
	if(is_tag()) return;
}

void component_info::resize(size_t new_capacity)
{
	if(is_tag()) return;

	memory::raw_region* new_region{new memory::raw_region(new_capacity * m_Size)};
	std::memcpy(new_region->data(), m_Region->data(), m_Capacity * m_Size);
	m_Capacity = new_capacity;
	delete(m_Region);
	m_Region = new_region;
}

uint64_t component_info::available() const noexcept { return m_Capacity - m_Entities.size(); }

bool component_info::is_tag() const noexcept { return m_Size == 0; }

psl::array<std::pair<entity, entity>> component_info::add(psl::array_view<std::pair<entity, entity>> entities)
{
	auto count = std::accumulate(std::begin(entities), std::end(entities), entity{0},
								 [](entity sum, const auto& range) { return sum + (range.second - range.first); });

	if(available() <= count) resize(std::max(m_Capacity * 2, m_Capacity + count));

	for(auto e_range : entities)
	{
		for(auto e = e_range.first; e < e_range.second; ++e)
		{
			assert(!m_Entities.has(e));
			m_Entities[e] = ((std::uintptr_t)m_Region->data() + (e * m_Size));
		}
	}

	return psl::array<std::pair<entity, entity>>{entities};
}

void component_info::add(psl::array_view<entity> entities)
{
	const auto count = entities.size();

	if(available() <= count) resize(std::max(m_Capacity * 2, m_Capacity + count));

	for(auto e : entities)
	{
		assert(!m_Entities.has(e));
		m_Entities[e] = ((std::uintptr_t)m_Region->data() + (e * m_Size));
	}
}

void component_info::destroy(psl::array_view<std::pair<entity, entity>> entities) noexcept
{
	for(auto e_range : entities)
	{
		m_Entities.erase(e_range.first, e_range.second);
	}
}
void component_info::destroy(psl::array_view<entity> entities) noexcept
{
	m_Entities.erase(std::begin(entities), std::end(entities));
}

void component_info::destroy(entity entity) noexcept { m_Entities.erase(entity); }
void component_info::purge() {}


void* component_info::data() const noexcept { return m_Region->data(); }


size_t component_info::id() const noexcept
{
	static_assert(sizeof(size_t) == sizeof(psl::ecs::details::component_key_t), "should be castable");
	return (size_t)m_ID;
}

psl::array_view<entity> component_info::entities() const noexcept { return m_Entities.indices(); }
bool component_info::has_component(entity e) const noexcept { return m_Entities.has(e); }

psl::array_view<std::uintptr_t> component_info::component_data() const noexcept
{
	return m_Entities.dense();
}