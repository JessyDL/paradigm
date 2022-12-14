
#ifdef SURFACE_WIN32

	#include "gfx/limits.hpp"
	#include "gles/context.hpp"
	#include "gles/igles.hpp"
	#include "logging.hpp"
	#include "os/surface.hpp"
	#include "psl/debug_utils.hpp"
	#include "psl/stream_utils.hpp"
	#include <Windows.h>

using namespace core;
using namespace core::igles;


HDC target;
HWND hwnd;
HGLRC rc;

void* glGetProcAddress(const char* name) {
	void* p = (void*)wglGetProcAddress(name);
	if(p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1)) {
		HMODULE module = LoadLibraryA("opengl32.dll");
		p			   = (void*)GetProcAddress(module, name);
	}

	return p;
}

context::context(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 psl::string8::view name) {
	auto window_data = cache.create<core::data::window>(1, 1);
	auto surface	 = cache.create<core::os::surface>(window_data);

	hwnd   = surface->surface_handle();
	target = GetDC(hwnd);


	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize	   = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags	   = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 32;

	int pfdid = ChoosePixelFormat(target, &pfd);
	if(pfdid == 0) {
		return;
	}

	if(SetPixelFormat(target, pfdid, &pfd) == false) {
		return;
	}

	int attriblist[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
						3,
						WGL_CONTEXT_MINOR_VERSION_ARB,
						2,
						WGL_CONTEXT_FLAGS_ARB,
	#ifdef PE_DEBUG
						WGL_CONTEXT_DEBUG_BIT_ARB |
	#endif
						  WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
						WGL_CONTEXT_PROFILE_MASK_ARB,
						WGL_CONTEXT_ES2_PROFILE_BIT_EXT,
						0};
	// rc = wglCreateContext(target); // Rendering Contex
	rc = wglCreateContext(target);	  // Rendering Contex
	if(!wglMakeCurrent(target, rc))
		return;

	int version = gladLoadWGL(target);
	if(!version) {
		core::igles::log->critical("could not create a context. failed to load fnpointers.");
		return;
	}
	version	   = gladLoadGLES2Loader((GLADloadproc)glGetProcAddress);
	auto error = glGetError();
	quey_capabilities();

	ReleaseDC(hwnd, target);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(rc);

	cache.free(surface);
	cache.free(window_data);
}

context::~context() {
	// make the rendering context not current
	wglMakeCurrent(NULL, NULL);

	// delete the rendering context
	wglDeleteContext(rc);
	ReleaseDC(hwnd, target);
}

static std::thread::id main_thread;

void GLAPIENTRY MessageCallback(GLenum source,
								GLenum type,
								GLuint id,
								GLenum severity,
								GLsizei length,
								const GLchar* message,
								const void* userParam) {
	switch(severity) {
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		core::igles::log->info("{0} - {1}: {2} at {3}", type, id, message, source);
		break;
	case GL_DEBUG_SEVERITY_LOW:
	case GL_DEBUG_SEVERITY_MEDIUM:
		if(type == GL_DEBUG_TYPE_ERROR)
			core::igles::log->error("{0} - {1}: {2} at {3}", type, id, message, source);
		else
			core::igles::log->warn("{0} - {1}: {2} at {3}", type, id, message, source);
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		if(type == GL_DEBUG_TYPE_ERROR) {
			auto stack = utility::debug::trace(0, 255, main_thread);
			psl::string traceStr {"--- TRACE ---\n"};
			for(const auto& trace : stack) {
				traceStr.append('\t' + utility::string::to_hex(trace.addr) + "    " + trace.name + '\n');
			}
			traceStr.append("--- END ---");
			core::igles::log->critical("{0} - {1}: {2} at {3}\n{4}", type, id, message, source, traceStr);
		} else
			core::igles::log->error("{0} - {1}: {2} at {3}", type, id, message, source);
		core::igles::log->flush();
		break;
	}
}

void context::enable(const core::os::surface& surface) {
	main_thread = std::this_thread::get_id();
	hwnd		= surface.surface_handle();
	target		= GetDC(hwnd);


	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize	   = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags	   = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 32;

	int pfdid = ChoosePixelFormat(target, &pfd);
	if(pfdid == 0) {
		return;
	}

	if(SetPixelFormat(target, pfdid, &pfd) == false) {
		return;
	}

	int attriblist[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
						3,
						WGL_CONTEXT_MINOR_VERSION_ARB,
						2,
						WGL_CONTEXT_FLAGS_ARB,
	#ifdef PE_DEBUG
						WGL_CONTEXT_DEBUG_BIT_ARB |
	#endif
						  WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
						WGL_CONTEXT_PROFILE_MASK_ARB,
						WGL_CONTEXT_ES2_PROFILE_BIT_EXT,
						0};
	// rc = wglCreateContext(target); // Rendering Contex
	rc = wglCreateContext(target);	  // Rendering Contex
	if(!wglMakeCurrent(target, rc))
		return;

	int version = gladLoadWGL(target);
	if(!version) {
		printf("Unable to load OpenGL\n");
		return;
	}
	version	   = gladLoadGLES2Loader((GLADloadproc)glGetProcAddress);
	auto error = glGetError();

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(rc);

	// pfdid = wglChoosePixelFormatEXT(target, &pfd);
	/*if(pfdid == 0)
	{
		return;
	}

	if(SetPixelFormat(target, pfdid, &pfd) == false)
	{
		return;
	}*/

	rc = wglCreateContextAttribsARB(target, 0, attriblist);
	if(!wglMakeCurrent(target, rc))
		return;
	version = gladLoadWGL(target);
	if(!version) {
		printf("Unable to load OpenGL\n");
		return;
	}
	version = gladLoadGLES2Loader((GLADloadproc)glGetProcAddress);
	error	= glGetError();

	auto glversion = glGetString(GL_VERSION);

	#ifdef PE_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	GLuint unusedIds = 0;
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
	#endif

	if(surface.data().buffering() != core::gfx::buffering::SINGLE) {
	#ifdef GL_EXT_swap_control_tear
		if(surface.data().buffering() == core::gfx::buffering::triple) {
			if(wglSwapIntervalEXT != NULL)
				wglSwapIntervalEXT(-1);
		} else {
			if(wglSwapIntervalEXT != NULL)
				wglSwapIntervalEXT(1);
		}
	#else
		if(wglSwapIntervalEXT != NULL)
			wglSwapIntervalEXT(1);
	#endif

		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		// glLineWidth(2.0);
	}
}

bool context::swapbuffers(core::resource::handle<core::os::surface> surface) {
	return SwapBuffers(target);
}
#endif
