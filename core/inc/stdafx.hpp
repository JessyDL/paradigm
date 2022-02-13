// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "psl/stdafx_psl.hpp"
#include "targetver.hpp"

#include "psl/platform_def.hpp"

#include "logging.hpp"
#include <stdio.h>


#include "psl/string_utils.hpp"
#include "psl/ustring.hpp"


#include "gfx/stdafx.hpp"

#include "psl/array_view.hpp"
#include "psl/enumerate.hpp"
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef PLATFORM_LINUX
// https://bugzilla.redhat.com/show_bug.cgi?id=130601 not a bug my ass, it's like the windows min/max..
#undef minor
#undef major
#endif


#include "psl/application_utils.hpp"

#include "conversion_utils.hpp"
#include "psl/library.hpp"
#include "psl/math/math.hpp"
#include "psl/meta.hpp"
#include "psl/serialization/serializer.hpp"
#include "resource/resource.hpp"

#include "data/stream.hpp"

#if defined(PLATFORM_WINDOWS)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>


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

static void chk_heap(char* file, int line)
{
	static const char* lastOkFile = "here";
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

/// \namespace core
/// \brief the engine core, here all graphics resources and render operations are described.

/// \namespace core::gfx 
/// \brief namespace that deals with the abstract render objects
/// \details using this namespace you can have an abstract API to the various supported graphics APIs

/// \namespace core::ivk 
/// \brief deals with all objects that are directly mapped to vulkan objects (i.e. they have internal vulkan concepts in them)

/// \namespace core::igles 
/// \brief OpenGLES API abstraction

/// \namespace core::resource 
/// \brief deals with resource creation, tracking, and managing.

/// \namespace core::meta 
/// \brief contains all extensions to the psl::meta namespace.

/// \namespace core::os 
/// \brief specific OS wrappers and resources.

/// \namespace core::systems 
/// \brief contains systems that are responsible for handling certain aspects (audio, input, physics, etc..).

/// \namespace core::ecs 
/// \brief All ECS related implementations

/// \namespace core::ecs::systems 
/// \brief ECS Systems

/// \namespace core::ecs::components 
/// \brief ECS Components