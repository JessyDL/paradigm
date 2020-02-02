#include "gfx/computepass.h"
#include "gfx/computecall.h"
#include "gfx/context.h"

#if PE_GLES
#include "gles/computepass.h"
#endif
#if PE_VULKAN
#include "vk/computepass.h"
#endif

using namespace core::resource;
using namespace core::gfx;
using namespace core;

computepass::computepass(handle<core::gfx::context> context)
{
	switch (context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_Handle = new core::igles::computepass();
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle = new core::ivk::computepass();
		break;
#endif
	}
}

computepass::~computepass()
{
	std::visit(utility::templates::overloaded{ [](auto&& pass) {return delete(pass); } }, m_Handle);
}


void computepass::prepare()
{
	return std::visit(utility::templates::overloaded{ [](auto&& pass) {return pass->prepare(); } }, m_Handle);
}
bool computepass::build(bool force)
{
	if (!m_Dirty && !force) return true;

	m_Dirty = false;
	return std::visit(utility::templates::overloaded{ [](auto&& pass) {return pass->build(); } }, m_Handle);
}


void computepass::clear()
{
	return std::visit(utility::templates::overloaded{ [](auto&& pass) {return pass->clear(); } }, m_Handle);
}
void computepass::present()
{
	return std::visit(utility::templates::overloaded{ [](auto&& pass) {return pass->present(); } }, m_Handle);
}


bool computepass::connect(psl::view_ptr<computepass> child) noexcept
{
	if (child->m_Handle.index() != m_Handle.index()) return false;

	if (m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::computepass*>(m_Handle);
		ptr->connect(psl::view_ptr<core::ivk::computepass>(std::get<core::ivk::computepass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::computepass*>(m_Handle);
		ptr->connect(psl::view_ptr<core::igles::computepass>(std::get<core::igles::computepass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif

	}
	return false;
}
bool computepass::disconnect(psl::view_ptr<computepass> child) noexcept
{
	if (child->m_Handle.index() != m_Handle.index()) return false;

	if (m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::computepass*>(m_Handle);
		ptr->disconnect(psl::view_ptr<core::ivk::computepass>(std::get<core::ivk::computepass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::computepass*>(m_Handle);
		ptr->disconnect(psl::view_ptr<core::igles::computepass>(std::get<core::igles::computepass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif

	}
	return false;
}


void computepass::add(const core::gfx::computecall& call) noexcept
{
	return std::visit(utility::templates::overloaded{ [&call](auto&& pass) {pass->add(call); } }, m_Handle);
}