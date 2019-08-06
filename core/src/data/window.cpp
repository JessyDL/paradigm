#include "data/window.h"
#include "resource/resource.hpp"
#include "meta.h"

using namespace psl;
using namespace core::data;
using namespace core::gfx;


window::window(const UID& uid, core::resource::cache& cache)
	: window::window()
{

}


window::window(const window& other, const UID& uid, core::resource::cache& cache, psl::string8::view name) noexcept
	: m_Width(other.m_Width), m_Height(other.m_Height), m_WindowMode(other.m_WindowMode),
	  m_Buffering(other.m_Buffering), m_Name(name)
{

}

window::window(uint32_t width, uint32_t height, surface_mode mode, core::gfx::buffering buffering, psl::string8::view name)
	: m_Width(width), m_Height(height), m_WindowMode(mode), m_Buffering(buffering), m_Name(name)
{

}



uint32_t window::width() const
{
	return m_Width.value;
}
void window::width(uint32_t width)
{
	m_Width = width;
}
uint32_t window::height() const
{
	return m_Height.value;
}
void window::height(uint32_t height)
{
	m_Height = height;
}
psl::string8::view window::name() const
{
	return m_Name.value;
}
void window::name(psl::string8::view name)
{
	m_Name = name;
}
core::gfx::surface_mode window::mode() const
{
	return m_WindowMode.value;
}
void window::mode(core::gfx::surface_mode mode)
{
	m_WindowMode = mode;
}
core::gfx::buffering window::buffering() const
{
	return m_Buffering.value;
}
void window::buffering(core::gfx::buffering mode)
{
	m_Buffering = mode;
}
