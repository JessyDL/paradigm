#include "ecs/details/component_info.h"
#include "ecs/entity.h"
#include "memory/raw_region.h"
#include <algorithm>
#include "pack_view.h"

using namespace psl::ecs;
using namespace psl::ecs::details;

component_info::component_info(component_key_t id, size_t size, size_t capacity)
	: m_Entities(), m_Region((size > 0)?new memory::raw_region(capacity * size): nullptr),
	  m_Generator((size> 0)?std::numeric_limits<uint64_t>::max(): 0), m_ID(id), m_Capacity((size>0)?capacity:0), m_Size(size)
{}

component_info::component_info(component_info&& other)
	: m_Entities(std::move(other.m_Entities)), m_Region(std::move(other.m_Region)),
	  m_Generator(std::move(other.m_Generator)), m_ID(std::move(other.m_ID)), m_Capacity(std::move(other.m_Capacity)),
	  m_Size(std::move(other.m_Size)){
	other.m_Region = nullptr;};

component_info::~component_info()
{
	if(m_Region != nullptr) delete(m_Region);
}

component_info& component_info::operator=(component_info&& other)
{
	if(this != &other)
	{
		m_Entities  = std::move(other.m_Entities);
		m_Region	= std::move(other.m_Region);
		m_Generator = std::move(other.m_Generator);
		m_ID		= std::move(other.m_ID);
		m_Capacity  = std::move(other.m_Capacity);
		m_Size		= std::move(other.m_Size);
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

psl::array<std::pair<uint64_t, uint64_t>> component_info::add(psl::array_view<entity> entities)
{
	if(available() <= entities.size())
		resize(std::max(m_Capacity * 2, m_Capacity + entities.size()));

	m_Entities.insert(std::end(m_Entities), std::begin(entities), std::end(entities));

	if(is_tag())
		return {};
	return m_Generator.create_multi(entities.size());
}

void component_info::destroy(psl::array_view<entity> entities, psl::array_view<uint64_t> indices)
{
	assert(entities.size() == indices.size());

	auto ib   = std::begin(entities);
	auto iter = std::remove_if(std::begin(m_Entities), std::end(m_Entities), [&ib, &entities](entity e) -> bool {
		while(ib != std::end(entities) && *ib < e) ++ib;
		return (ib != std::end(entities) && *ib == e);
	});

	assert(std::distance(iter, std::end(m_Entities)) == entities.size());

	psl::array<std::pair<uint64_t, uint64_t>> collapsed_indices;

	uint64_t range_start = indices[0];
	uint64_t range_end   = indices[0] + 1;
	for(auto i = size_t{1}; i < indices.size(); ++i)
	{
		if(indices[i] != range_end)
		{
			collapsed_indices.emplace_back(range_start, range_end - range_start);
			range_start = indices[i];
			range_end   = range_start;
		}
		range_end += 1;
	}
	collapsed_indices.emplace_back(range_start, range_end - range_start);
	m_MarkedForDeletion.insert(std::end(m_MarkedForDeletion), std::begin(collapsed_indices),
							   std::end(collapsed_indices));
}

void component_info::purge()
{
	for(auto& range : m_MarkedForDeletion) m_Generator.destroy(range.first, range.second);
}


psl::array_view<entity> component_info::entities() const noexcept { return m_Entities; }
psl::array_view<entity> component_info::deleted_entities() const noexcept { return m_DeletedEntities; }

void* component_info::data() const noexcept { return m_Region->data(); }


size_t component_info::id() const noexcept
{
	static_assert(sizeof(size_t) == sizeof(psl::ecs::details::component_key_t), "should be castable");
	return (size_t)m_ID;
}