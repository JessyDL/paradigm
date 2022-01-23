#include "gles/swapchain.hpp"
#include "gles/igles.hpp"
#include "gles/context.hpp"
#include "os/surface.hpp"

using namespace core::igles;
using namespace core;
using namespace core::resource;

swapchain::swapchain(core::resource::cache_t& cache,
					 const core::resource::metadata& metaData,
					 psl::meta::file* metaFile,
					 core::resource::handle<core::os::surface> surface,
					 core::resource::handle<core::igles::context> context,
					 bool use_depth) :
	m_Surface(surface),
	m_Context(context), m_UseDepth(use_depth)
{
	// todo support srgb;
	context->enable(surface.value());
	glViewport(0, 0, static_cast<GLsizei>(surface->data().width()), static_cast<GLsizei>(surface->data().height()));
	if(m_UseDepth)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
	}
}

swapchain::~swapchain() {}

bool swapchain::present() { return m_Context->swapbuffers(m_Surface); }

void swapchain::clear()
{
	if(m_UseDepth)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
		glClearDepthf(m_ClearDepth);
		glClearStencil(m_ClearStencil);
	}
	else
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
	}
}