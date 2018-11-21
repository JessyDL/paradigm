#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <memory>
#include "profiling/profiler.h"

namespace core
{
	extern std::shared_ptr<spdlog::logger> log;
	namespace gfx
	{
		extern std::shared_ptr<spdlog::logger> log;
	}
	namespace ivk
	{
		extern std::shared_ptr<spdlog::logger> log;
	}
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
}