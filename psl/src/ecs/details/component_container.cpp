#include "psl/ecs/details/component_container.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/pack_view.hpp"
#include <algorithm>
#include <numeric>

using namespace psl::ecs;
using namespace psl::ecs::details;

component_container_t::component_container_t(component_type_info_t info) : m_Info(info) {}

component_container_t::component_container_t(component_container_t&& other) : m_Info(std::move(other.m_Info)) {};


component_container_t& component_container_t::operator=(component_container_t&& other) {
	if(this != &other) {
		m_Info = std::move(other.m_Info);
	}
	return *this;
}

bool component_container_t::is_flag() const noexcept {
	return m_Info.size == 0;
}
