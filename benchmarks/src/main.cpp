#include "stdafx_tests.h"

#ifdef PLATFORM_WINDOWS
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif

#include "spdlog/spdlog.h"

void setup_loggers()
{
	std::vector<spdlog::sink_ptr> sinks;
	auto logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
	spdlog::register_logger(logger);
	core::log = logger;
	

	auto system_logger = std::make_shared<spdlog::logger>("systems", begin(sinks), end(sinks));
	spdlog::register_logger(system_logger);
	core::systems::log = system_logger;


	auto os_logger = std::make_shared<spdlog::logger>("os", begin(sinks), end(sinks));
	spdlog::register_logger(os_logger);
	core::os::log = os_logger;


	auto data_logger = std::make_shared<spdlog::logger>("data", begin(sinks), end(sinks));
	spdlog::register_logger(data_logger);
	core::data::log = data_logger;


	auto gfx_logger = std::make_shared<spdlog::logger>("gfx", begin(sinks), end(sinks));
	spdlog::register_logger(gfx_logger);
	core::gfx::log = gfx_logger;


	auto ivk_logger = std::make_shared<spdlog::logger>("ivk", begin(sinks), end(sinks));
	spdlog::register_logger(ivk_logger);
	core::ivk::log = ivk_logger;
}

int main(int argc, char* argv[]) 
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	return 0;
}
