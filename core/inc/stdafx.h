// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "targetver.h"
#include "stdafx_psl.h"

#include "platform_def.h"

#include <stdio.h>
#include "logging.h"

//#define GLM_FORCE_MESSAGES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_INLINE
#define GLM_FORCE_SIZE_T_LENGTH
#ifdef __clang__
#define GLM_FORCE_CXX14
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/bbox.h>
#include "glmUtility.h"


#define VULKAN_HPP_NO_EXCEPTIONS
#include "vulkan_stdafx.h"

#include <vector>
#include <optional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>
#include <memory>

#ifdef PLATFORM_LINUX
// https://bugzilla.redhat.com/show_bug.cgi?id=130601 not a bug my ass, it's like the windows min/max..
#undef minor
#undef major
#endif


#include "string_utils.h"
#include "application_utils.h"

#include "meta.h"
#include "library.h"
#include "systems/resource.h"
#include "math/math.hpp"

#include "data/stream.h"

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


#ifndef DBG_NEW
#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW
#endif

#endif

#ifdef assert
#undef assert
#endif

#if defined(RELEASE) || !defined(PLATFORM_WINDOWS)

#define assert(expression) ((void)0)

#else

_ACRTIMP void __cdecl _wassert(_In_z_ wchar_t const* _Message, _In_z_ wchar_t const* _File, _In_ unsigned _Line);

#define assert(expression)                                                                                             \
	(void)((!!(expression)) || (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0))

#endif

#define STRINGIZE_HELPER(x) #x
#define STRINGIZE(x) STRINGIZE_HELPER(x)
#define CMP_ISSUE(number, desc)                                                                                        \
	__pragma(message("ISSUE: " #number "\nDescription: " #desc "\n" __FILE__ "(" STRINGIZE(__LINE__) ")"))
#define CMP_ERROR(number, desc)                                                                                        \
	__pragma(message("error: " #number "\nDescription: " #desc "\n" __FILE__ "(" STRINGIZE(__LINE__) ")"))
#define CMP_WARNING(number, desc)                                                                                      \
	__pragma(message("warning: " #number "\nDescription: " #desc "\n" __FILE__ "(" STRINGIZE(__LINE__) ")"))
#define CMP_TODO(number, desc)                                                                                         \
	__pragma(message("TODO: " #number "\nDescription: " #desc "\n" __FILE__ "(" STRINGIZE(__LINE__) ")"))


#include "Logger.h"

//#ifdef DEBUG
#include <malloc.h>

#if defined(PLATFORM_WINDOWS)

#define CHKHEAP() (chk_heap(__FILE__, __LINE__))

static void chk_heap(char *file, int line)
{
	static const char *lastOkFile = "here";
	static int lastOkLine		  = 0;
	static int heapOK			  = 1;

	if(!heapOK) return;

	if(_heapchk() == _HEAPOK)
	{
		lastOkFile = file;
		lastOkLine = line;
		return;
	}

	heapOK = 0;
	printf("Heap corruption detected at %s (%d)\n", file, line);
	printf("Last OK at %s (%d)\n", lastOkFile, lastOkLine);
}
#endif

//#endif
/*
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
return malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned
debugFlags, const char* file, int line)
{
return malloc(size);
}
*/

/*! \namespace core \brief the engine core, here all graphics resources and render operations are described.

*/

/*! \namespace core::gfx \brief namespace that deals with the abstract render objects
	\note currently this namespace is shared with core::ivk, this will change in the future however.
*/
/*! \namespace core::ivk \brief deals with all objects that are directly mapped to vulkan objects (i.e. they have internal vulkan concepts in them)

*/

/*! \namespace core::resource \brief deals with resource creation, tracking, and managing.

*/

/*! \namespace core::meta \brief contains all extensions to the ::meta namespace.

*/
/*! \namespace core::os \brief specific OS wrappers and resources.

*/
/*! \namespace core::systems \brief contains systems that are responsible for handling certain aspects (audio, input, physics, etc..).

*/