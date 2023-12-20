#pragma once

#ifdef SURFACE_WIN32
	// including wgl first as it also includes windows.h, not doing so creates warnings about redefinition of some
	// defines
	#include "glad/glad.h"
	#include "glad/glad_wgl.h"
#endif
#ifdef SURFACE_XCB
	#include <GLES3/gl32.h>
	#include <GLES3/gl3ext.h>
// #include "glad/glad_egl.h"
#endif

inline auto glGetStringView(GLenum name) -> psl::string_view {
	return psl::string_view {reinterpret_cast<const char*>(glGetString(name))};
}
