#pragma once

// note that assembler could be including this file through the `gles/conversion.hpp`, and if PE_GLES is not defined
// we will be getting the symbols from assembler instead.
// This define guards against paradigm's core from being affected by assembler's build settings.
#if defined(PE_GLES)
	#if defined(SURFACE_WIN32)
		// including wgl first as it also includes windows.h, not doing so creates warnings about redefinition of some
		// defines
		#include "glad/wgl.h"

		// note: added extra space here to satisfy the earlier comment and to avoid any include reordering to affect
		// this
		#include "glad/gles2.h"
	#elif defined(SURFACE_XCB)
		#include <GLES3/gl32.h>
		#include <GLES3/gl3ext.h>
	// #include "glad/glad_egl.h"
	#endif

inline auto glGetStringView(GLenum name) -> psl::string_view {
	return psl::string_view {reinterpret_cast<const char*>(glGetString(name))};
}
#endif
