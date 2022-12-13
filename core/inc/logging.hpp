#pragma once
#include "psl/logging.hpp"
#include "psl/profiling/profiler.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <memory>

namespace core
{
extern std::shared_ptr<spdlog::logger> log;
namespace gfx
{
	extern std::shared_ptr<spdlog::logger> log;
}
#ifdef PE_VULKAN
namespace ivk
{
	extern std::shared_ptr<spdlog::logger> log;
}
#endif
#ifdef PE_GLES
namespace igles
{
	extern std::shared_ptr<spdlog::logger> log;
}
#endif
namespace data
{
	extern std::shared_ptr<spdlog::logger> log;
}
namespace systems
{
	extern std::shared_ptr<spdlog::logger> log;
}
namespace os
{
	extern std::shared_ptr<spdlog::logger> log;
}

extern psl::profiling::profiler profiler;
}	 // namespace core