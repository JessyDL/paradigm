#pragma once
#include "spdlog/spdlog.h"


#ifndef DISABLE_LOGGING
	#define LOG(...) spdlog::get("main")->info(__VA_ARGS__)
	#define LOG_INFO(...) LOG(__VA_ARGS__)
	#define LOG_ERROR(...) spdlog::get("main")->error(__VA_ARGS__)
	#define LOG_WARNING(...) spdlog::get("main")->warn(__VA_ARGS__)
	#define LOG_PROFILE(...) spdlog::get("main")->trace(__VA_ARGS__)
	#define LOG_DEBUG(...) spdlog::get("main")->debug(__VA_ARGS__)
	#define LOG_FATAL(...) spdlog::get("main")->critical(__VA_ARGS__)
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
