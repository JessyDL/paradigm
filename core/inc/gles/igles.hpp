#pragma once

#ifdef SURFACE_WIN32
#include "glad/glad.h"
#include "glad/glad_wgl.h"
#endif
#ifdef SURFACE_XCB
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
//#include "glad/glad_egl.h"
#endif