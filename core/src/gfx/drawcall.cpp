#include "stdafx.h"
#include "gfx/drawcall.h"
#include <algorithm>

using namespace core::gfx;
using namespace core::resource;
drawcall::drawcall(handle<core::gfx::material> material, const std::vector<handle<geometry>>& geometry) noexcept
	: m_Material(material), m_Geometry(geometry)
{}


bool drawcall::add(handle<geometry> geometry) noexcept
{
	if(std::find_if(std::begin(m_Geometry), std::end(m_Geometry),
					[&geometry](const handle<core::gfx::geometry>& geomHandle) {
		   return geomHandle.ID() == geometry.ID();
	   }) == std::end(m_Geometry))
	{
		m_Geometry.push_back(geometry);
		return true;
	}
	return false;
}
bool drawcall::remove(core::resource::handle<core::gfx::geometry> geometry) noexcept
{
	if(auto it = std::find_if(
		   std::begin(m_Geometry), std::end(m_Geometry),
		   [&geometry](const handle<core::gfx::geometry>& geomHandle) {
		   return geomHandle.ID() == geometry.ID();
	   }); it != std::end(m_Geometry))
	{
		m_Geometry.erase(it);
		return true;
	}
	return false;
}
bool drawcall::remove(const UID& geometry) noexcept
{
	if(auto it = std::find_if(
		   std::begin(m_Geometry), std::end(m_Geometry),
		   [&geometry](const handle<core::gfx::geometry>& geomHandle) { return geomHandle.ID() == geometry; });
	   it != std::end(m_Geometry))
	{
		m_Geometry.erase(it);
		return true;
	}
	return false;
}
void drawcall::material(handle<core::gfx::material> material) noexcept {
	m_Material = material; }
core::resource::handle<core::gfx::material> drawcall::material() const noexcept { return m_Material;}