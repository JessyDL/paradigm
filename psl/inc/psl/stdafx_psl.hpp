#pragma once

#define __STDC_WANT_LIB_EXT1__ 1	// https://stackoverflow.com/questions/31278172/undefined-reference-to-memcpy-s
#include <array>
#include <cstddef>
#include <stdarg.h>
#include <stddef.h>
#include <vector>

#include "psl/platform_def.hpp"
#include "psl/ustring.hpp"

#ifdef PLATFORM_ANDROID
#include <android/configuration.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#endif

#include "psl/logging.hpp"

namespace platform::specifics
{
#if defined(PLATFORM_ANDROID)
	extern android_app* android_application;
#endif

}	 // namespace platform::specifics


#if !defined(NDEBUG) && defined(NDEBUG) && !defined(DEBUG)
#define NDEBUG 1
#endif

#include "psl/assertions.hpp"

/*! \namespace memory \brief this namespace contains types and utilities for managing regions of memory
	\details due to the potential constrained environments that this app might run in, there are
	some helper classes provided to track and manage memory (both physically backed and non-physically backed).
	You can find more information in memory::region about this.
*/


#include "psl/profiling/profiler.hpp"
#include "psl/template_utils.hpp"
#include "psl/timer.hpp"

#include "psl/array_view.hpp"
#include "psl/enumerate.hpp"
#include "psl/pack_view.hpp"

#include "psl/literals.hpp"