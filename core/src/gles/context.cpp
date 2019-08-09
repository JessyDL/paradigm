#include "gles/context.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "os/surface.h"
#include "glad/glad_wgl.h"

HDC target;
HWND hwnd;
HGLRC rc;

using namespace core;
using namespace core::igles;

void* glGetProcAddress(const char* name)
{
	void* p = (void*)wglGetProcAddress(name);
	if(p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1))
	{
		HMODULE module = LoadLibraryA("opengl32.dll");
		p			   = (void*)GetProcAddress(module, name);
	}

	return p;
}

context::context(const psl::UID& uid, core::resource::cache& cache, psl::string8::view name) {}

context::~context()
{
	// make the rendering context not current
	wglMakeCurrent(NULL, NULL);

	// delete the rendering context
	wglDeleteContext(rc);
	ReleaseDC(hwnd, target);
}

void context::enable(const core::os::surface& surface)
{
	hwnd   = surface.surface_handle();
	target = GetDC(hwnd);


	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize	  = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags	= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;

	const int pfdid = ChoosePixelFormat(target, &pfd);
	if(pfdid == 0)
	{
		return;
	}

	if(SetPixelFormat(target, pfdid, &pfd) == false)
	{
		return;
	}

	rc = wglCreateContext(target); // Rendering Contex
	if(!wglMakeCurrent(target, rc)) return;


	int version = gladLoadWGL(target);
	if(!version)
	{
		printf("Unable to load OpenGL\n");
		return;
	}
	version = gladLoadGLES2Loader((GLADloadproc)glGetProcAddress);


	if(surface.data().buffering() != core::gfx::buffering::SINGLE)
	{

#ifdef GL_EXT_swap_control_tear
		if(surface.data().buffering() == core::gfx::buffering::triple)
		{
			if(wglSwapIntervalEXT != NULL) wglSwapIntervalEXT(-1);
		}
		else
		{
			if(wglSwapIntervalEXT != NULL) wglSwapIntervalEXT(1);
		}
#else
		if(wglSwapIntervalEXT != NULL) wglSwapIntervalEXT(1);
#endif
	}
}


bool context::swapbuffers(core::resource::handle<core::os::surface> surface) { return SwapBuffers(target); }