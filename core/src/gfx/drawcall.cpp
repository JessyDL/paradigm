#include "gfx/drawcall.h"
#include <algorithm>
#include "systems/resource.h"

using namespace psl;
using namespace core::gfx;
using namespace core::resource;
drawcall::drawcall(handle<core::gfx::material> material, const std::vector<handle<geometry>>& geometry) noexcept
	: m_Material(material)
{
	for(auto& g : geometry)
		m_Geometry.emplace_back(g, 1);
}


bool drawcall::add(handle<geometry> geometry) noexcept
{
	if(auto it = std::find_if(std::begin(m_Geometry), std::end(m_Geometry),
					[&geometry](const std::pair<handle<core::gfx::geometry>, size_t>& geomHandle) {
		   return geomHandle.first.ID() == geometry.ID();
	   }); it == std::end(m_Geometry))
	{
		m_Geometry.emplace_back(std::make_pair(geometry, size_t{1}));
		return true;
	}
	else
	{
		it->second += 1;
	}
	return false;
}
bool drawcall::remove(core::resource::handle<core::gfx::geometry> geometry) noexcept
{
	if(auto it = std::find_if(
		   std::begin(m_Geometry), std::end(m_Geometry),
		   [&geometry](const std::pair<handle<core::gfx::geometry>, size_t>& geomHandle) {
		   return geomHandle.first.ID() == geometry.ID();
	   }); it != std::end(m_Geometry))
	{
		it->second -= 1;
		if(it->second == 0)
			m_Geometry.erase(it);

		return true;
	}
	return false;
}
bool drawcall::remove(const UID& geometry) noexcept
{
	if(auto it = std::find_if(
		   std::begin(m_Geometry), std::end(m_Geometry),
		   [&geometry](const std::pair<handle<core::gfx::geometry>, size_t>& geomHandle) { return geomHandle.first.ID() == geometry; });
	   it != std::end(m_Geometry))
	{
		it->second -= 1;
		if(it->second == 0)
			m_Geometry.erase(it);
		return true;
	}
	return false;
}
void drawcall::material(handle<core::gfx::material> material) noexcept {
	m_Material = material; }
core::resource::handle<core::gfx::material> drawcall::material() const noexcept { return m_Material;}