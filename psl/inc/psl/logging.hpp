#pragma once
#include "spdlog/spdlog.h"


#ifndef DISABLE_LOGGING
	#define LOG(...) spdlog::get("core")->info(__VA_ARGS__)
	#define LOG_INFO(...) LOG(__VA_ARGS__)
	#define LOG_ERROR(...) spdlog::get("core")->error(__VA_ARGS__)
	#define LOG_WARNING(...) spdlog::get("core")->warn(__VA_ARGS__)
	#define LOG_PROFILE(...) spdlog::get("core")->trace(__VA_ARGS__)
	#define LOG_DEBUG(...) spdlog::get("core")->debug(__VA_ARGS__)
	#define LOG_FATAL(...) spdlog::get("core")->critical(__VA_ARGS__)
#else
	#define TimeLog
	#define LOG
	#define LOG_INFO
	#define LOG_ERROR
	#define LOG_WARNING
	#define LOG_PROFILE
	#define LOG_DEBUG
	#define LOG_FATAL
#endif
