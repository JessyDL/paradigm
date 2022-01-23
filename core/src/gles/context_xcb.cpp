
#if defined(SURFACE_XCB)

#include "gfx/limits.hpp"
#include "gles/context.hpp"
#include "gles/igles.hpp"
#include "logging.hpp"
#include "os/surface.hpp"

#include <EGL/egl.h>
#include <GLES3/gl3platform.h>

using namespace core;
using namespace core::igles;

context::context(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 psl::string8::view name)
{
	auto window_data = cache.create<core::data::window>(1, 1);
	auto os_surface	 = cache.create<core::os::surface>(window_data);

	auto window = os_surface->surface_handle();


	/* get an EGL display connection */
	data.egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(!eglInitialize(data.egl_display, NULL, NULL))
	{
		core::igles::log->error("eglInitialize() failed");
		return;
	}
	EGLint const attribute_list[] = {EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1, EGL_NONE};
	EGLint num_config;
	if(!eglChooseConfig(data.egl_display, attribute_list, &data.egl_config, 1, &num_config))
	{
		core::igles::log->error("eglChooseConfig() failed");
		return;
	}

	if(!eglBindAPI(EGL_OPENGL_ES_API))
	{
		core::igles::log->error("eglBindAPI() failed");
		return;
	}

	const EGLint ctx_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
	data.egl_context		   = eglCreateContext(data.egl_display, data.egl_config, EGL_NO_CONTEXT, ctx_attribs);

	EGLint eglConfAttrVisualID;
	if(!eglGetConfigAttrib(data.egl_display, data.egl_config, EGL_NATIVE_VISUAL_ID, &eglConfAttrVisualID))
	{
		core::igles::log->error("eglGetConfigAttrib() failed");
		return;
	}

	data.egl_surface = eglCreateWindowSurface(data.egl_display, data.egl_config, os_surface->surface_handle(), NULL);
	eglMakeCurrent(data.egl_display, data.egl_surface, data.egl_surface, data.egl_context);

	GLint value {};

	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &value);
	m_Limits.storage.alignment = value;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &value);
	m_Limits.storage.size = value;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &value);
	m_Limits.uniform.alignment = value;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &value);
	m_Limits.uniform.size = value;

#ifdef GL_ARB_map_buffer_alignment
	glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &value);
	m_Limits.memorymap.alignment = value;
#else
	// no real requirements, but let's take something
	m_Limits.memorymap.alignment = 4;
#endif
	m_Limits.memorymap.size		   = std::numeric_limits<uint64_t>::max();
	m_Limits.supported_depthformat = core::gfx::format_t::d32_sfloat;

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &value);
	m_Limits.compute.workgroup.count[0] = value;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &value);
	m_Limits.compute.workgroup.count[1] = value;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &value);
	m_Limits.compute.workgroup.count[2] = value;

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &value);
	m_Limits.compute.workgroup.size[0] = value;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &value);
	m_Limits.compute.workgroup.size[1] = value;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &value);
	m_Limits.compute.workgroup.size[2] = value;

	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &value);
	m_Limits.compute.workgroup.invocations = value;

	core::igles::log->info("GLES context initialized");
	core::igles::log->info("vendor: {}", glGetString(GL_VENDOR));
	core::igles::log->info("renderer: {}", glGetString(GL_RENDERER));
	core::igles::log->info("version: {}", glGetString(GL_VERSION));

	eglMakeCurrent(data.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(data.egl_display, data.egl_surface);
	eglDestroyContext(data.egl_display, data.egl_context);
	eglTerminate(data.egl_display);

	data = {};
}

context::~context()
{
	eglMakeCurrent(data.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(data.egl_display, data.egl_surface);
	eglDestroyContext(data.egl_display, data.egl_context);
	eglTerminate(data.egl_display);
}

void context::enable(const core::os::surface& surface)
{
	if(data.egl_context != nullptr)
	{
		eglMakeCurrent(data.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroySurface(data.egl_display, data.egl_surface);
		eglDestroyContext(data.egl_display, data.egl_context);
	}
	data.egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(!eglInitialize(data.egl_display, NULL, NULL))
	{
		core::igles::log->error("eglInitialize() failed");
		return;
	}

	EGLint const attribute_list[] = {EGL_RED_SIZE,
									 8,
									 EGL_GREEN_SIZE,
									 8,
									 EGL_BLUE_SIZE,
									 8,
									 EGL_ALPHA_SIZE,
									 8,
									 EGL_BUFFER_SIZE,
									 32,
									 EGL_DEPTH_SIZE,
									 32,
									 EGL_RENDERABLE_TYPE,
									 EGL_OPENGL_ES3_BIT,
									 EGL_NONE};

	EGLint num_config;
	if(!eglChooseConfig(data.egl_display, attribute_list, &data.egl_config, 1, &num_config))
	{
		core::igles::log->error("eglChooseConfig() failed");
		return;
	}

	if(!eglBindAPI(EGL_OPENGL_ES_API))
	{
		core::igles::log->error("eglBindAPI() failed");
		return;
	}

	const EGLint ctx_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
	data.egl_context		   = eglCreateContext(data.egl_display, data.egl_config, EGL_NO_CONTEXT, ctx_attribs);


	EGLint eglConfAttrVisualID;
	if(!eglGetConfigAttrib(data.egl_display, data.egl_config, EGL_NATIVE_VISUAL_ID, &eglConfAttrVisualID))
	{
		core::igles::log->error("eglGetConfigAttrib() failed");
		return;
	}

	data.egl_surface = eglCreateWindowSurface(data.egl_display, data.egl_config, surface.surface_handle(), NULL);
	eglMakeCurrent(data.egl_display, data.egl_surface, data.egl_surface, data.egl_context);
}

bool context::swapbuffers(core::resource::handle<core::os::surface> surface)
{
	return eglSwapBuffers(data.egl_display, data.egl_surface);
}
#endif