#include "psl/ecs/details/component_container.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/pack_view.hpp"
#include <algorithm>
#include <numeric>

using namespace psl::ecs;
using namespace psl::ecs::details;

component_container_t::component_container_t(const component_key_t& id, size_t size, size_t alignment)
	: m_ID(std::move(id)), m_Size(size), m_Alignment(alignment) {}

component_container_t::component_container_t(component_container_t&& other)
	: m_ID(std::move(other.m_ID)), m_Size(std::move(other.m_Size)) {};


component_container_t& component_container_t::operator=(component_container_t&& other) {
	if(this != &other) {
		m_ID   = std::move(other.m_ID);
		m_Size = std::move(other.m_Size);
	}
	return *this;
}

bool component_container_t::is_tag() const noexcept {
	return m_Size == 0;
}


const component_key_t& component_container_t::id() const noexcept {
	return m_ID;
}
