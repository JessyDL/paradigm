#pragma once

#define __STDC_WANT_LIB_EXT1__ 1 // https://stackoverflow.com/questions/31278172/undefined-reference-to-memcpy-s
#include <stddef.h>
#include <stdarg.h>
#include <cstddef>
#include <vector>
#include <array>

#include "platform_def.h"
#include "ustring.h"

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/configuration.h>
#endif

#include "logging.h"

namespace platform::specifics
{
#if defined(PLATFORM_ANDROID)
	extern android_app* android_application;
#endif

} // namespace platform::specifics


#if !defined(NDEBUG) && defined(NDEBUG) && !defined(DEBUG)
#define NDEBUG 1
#endif

#include "assertions.h"

/*! \namespace memory \brief this namespace contains types and utilities for managing regions of memory
	\details due to the potential constrained environments that this app might run in, there are
	some helper classes provided to track and manage memory (both physically backed and non-physically backed).
	You can find more information in memory::region about this.
*/


#include "template_utils.h"
#include "timer.h"
#include "profiling/profiler.h"

#include "enumerate.h"
#include "array_view.h"
#include "pack_view.h"