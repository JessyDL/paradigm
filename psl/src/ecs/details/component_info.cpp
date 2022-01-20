#include "psl/ecs/details/component_info.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/pack_view.hpp"
#include <algorithm>
#include <numeric>

using namespace psl::ecs;
using namespace psl::ecs::details;

component_info::component_info(component_key_t id, size_t size) : m_ID(id), m_Size(size) {}

component_info::component_info(component_info&& other) :
	m_ID(std::move(other.m_ID)), m_Size(std::move(other.m_Size)) {};


component_info& component_info::operator=(component_info&& other)
{
	if(this != &other)
	{
		m_ID   = std::move(other.m_ID);
		m_Size = std::move(other.m_Size);
	}
	return *this;
}

bool component_info::is_tag() const noexcept { return m_Size == 0; }


component_key_t component_info::id() const noexcept { return m_ID; }
