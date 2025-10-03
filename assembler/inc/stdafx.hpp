// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define NO_MIN_MAX
#undef MIN
#undef MAX
#include "psl/string_utils.hpp"
#include "psl/ustring.hpp"

#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace assembler {
extern std::shared_ptr<spdlog::logger> log;
}

// TODO: reference additional headers your program requires here
