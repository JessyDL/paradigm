#include "gles/swapchain.h"
#include "gles/context.h"
#include "os/surface.h"
#include "glad/glad_wgl.h"

using namespace core::igles;
using namespace core;
using namespace core::resource;

swapchain::swapchain(const psl::UID& uid, core::resource::cache& cache,
					 core::resource::handle<core::os::surface> surface,
					 core::resource::handle<core::igles::context> context, bool use_depth)
	: m_Surface(surface), m_Context(context)
{
	context->enable(surface);
}

swapchain::~swapchain() {}

bool swapchain::present() { return m_Context->swapbuffers(m_Surface); }