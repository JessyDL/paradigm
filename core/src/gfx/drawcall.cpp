#include "gfx/drawcall.hpp"
#include "gfx/geometry.hpp"
#include "resource/resource.hpp"
#include <algorithm>


using namespace psl;
using namespace core::gfx;
using namespace core::ivk;
using namespace core::resource;
drawcall::drawcall(handle<core::gfx::bundle> bundle, const std::vector<handle<core::gfx::geometry_t>>& geometry) noexcept
	:
	m_Bundle(bundle)
{
	for(auto& g : geometry) m_Geometry.emplace_back(g, 1);
}


bool drawcall::add(handle<core::gfx::geometry_t> geometry) noexcept
{
	if(auto it = std::find_if(std::begin(m_Geometry),
							  std::end(m_Geometry),
							  [&geometry](const std::pair<handle<core::gfx::geometry_t>, size_t>& geomHandle) {
								  return geomHandle.first == geometry;
							  });
	   it == std::end(m_Geometry))
	{
		m_Geometry.emplace_back(std::make_pair(geometry, size_t {1}));
		return true;
	}
	else
	{
		it->second += 1;
	}
	return false;
}
bool drawcall::remove(core::resource::handle<core::gfx::geometry_t> geometry) noexcept
{
	if(auto it = std::find_if(std::begin(m_Geometry),
							  std::end(m_Geometry),
							  [&geometry](const std::pair<handle<core::gfx::geometry_t>, size_t>& geomHandle) {
								  return geomHandle.first == geometry;
							  });
	   it != std::end(m_Geometry))
	{
		it->second -= 1;
		if(it->second == 0) m_Geometry.erase(it);

		return true;
	}
	return false;
}
bool drawcall::remove(const UID& geometry) noexcept
{
	if(auto it = std::find_if(std::begin(m_Geometry),
							  std::end(m_Geometry),
							  [&geometry](const std::pair<handle<core::gfx::geometry_t>, size_t>& geomHandle) {
								  return geomHandle.first == geometry;
							  });
	   it != std::end(m_Geometry))
	{
		it->second -= 1;
		if(it->second == 0) m_Geometry.erase(it);
		return true;
	}
	return false;
}
void drawcall::bundle(handle<core::gfx::bundle> bundle) noexcept { m_Bundle = bundle; }
core::resource::handle<core::gfx::bundle> drawcall::bundle() const noexcept { return m_Bundle; }
