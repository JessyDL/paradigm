#pragma once

#define __STDC_WANT_LIB_EXT1__ 1	// https://stackoverflow.com/questions/31278172/undefined-reference-to-memcpy-s
#include <array>
#include <cstddef>
#include <stdarg.h>
#include <stddef.h>
#include <vector>

#include "psl/platform_def.hpp"
#include "psl/ustring.hpp"

#include "psl/logging.hpp"


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
