// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "targetver.h"
#include "psl/stdafx_psl.h"

#include "psl/platform_def.h"

#include <stdio.h>
#include "logging.h"


#include "psl/ustring.h"
#include "psl/string_utils.h"


#include "gfx/stdafx.h"

#include <vector>
#include <optional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>
#include <memory>
#include "psl/array_view.h"
#include "psl/enumerate.h"

#ifdef PLATFORM_LINUX
// https://bugzilla.redhat.com/show_bug.cgi?id=130601 not a bug my ass, it's like the windows min/max..
#undef minor
#undef major
#endif


#include "psl/application_utils.h"

#include "psl/meta.h"
#include "psl/library.h"
#include "psl/serialization.h"
#include "resource/resource.hpp"
#include "psl/math/math.hpp"
#include "conversion_utils.h"

#include "data/stream.h"

#if defined(PLATFORM_WINDOWS)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


//#ifndef DBG_NEW
//#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
//#define new DBG_NEW
//#endif

#endif

#ifdef RELEASE
#ifdef assert
#undef assert

#define assert ()
#endif
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