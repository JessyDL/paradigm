#include "core/logging.hpp"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#ifdef _MSC_VER
	#include "spdlog/sinks/msvc_sink.h"
#endif

std::shared_ptr<spdlog::logger> core::log {nullptr};
std::shared_ptr<spdlog::logger> core::gfx::log {nullptr};
#ifdef PE_VULKAN
std::shared_ptr<spdlog::logger> core::ivk::log {nullptr};
#endif
#ifdef PE_GLES
std::shared_ptr<spdlog::logger> core::igles::log {nullptr};
#endif
#ifdef PE_WEBGPU
std::shared_ptr<spdlog::logger> core::iwgpu::log {nullptr};
#endif
std::shared_ptr<spdlog::logger> core::data::log {nullptr};
std::shared_ptr<spdlog::logger> core::systems::log {nullptr};
std::shared_ptr<spdlog::logger> core::os::log {nullptr};


psl::profiling::profiler core::profiler {};

#ifndef PE_PLATFORM_ANDROID

inline std::tm localtime_safe(std::time_t timer) {
	std::tm bt {};
	#if defined(__unix__)
	localtime_r(&timer, &bt);
	#elif defined(_MSC_VER)
	localtime_s(&bt, &timer);
	#else
	static std::mutex mtx;
	std::lock_guard<std::mutex> lock(mtx);
	bt = *std::localtime(&timer);
	#endif
	return bt;
}

auto core::initialize_loggers() -> bool {
	if(core::_loggers_initialized)
		return true;

	core::_loggers_initialized = true;

	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c						  = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm							  = localtime_safe(now_c);
	psl::string time;
	time.resize(20);
	strftime(time.data(), 20, "%Y-%m-%d %H-%M-%S", &now_tm);
	time[time.size() - 1] = '/';
	psl::string sub_path  = "logs/" + time;
	if(!psl::utility::platform::file::exists(psl::utility::application::path::get_path() + sub_path + "main.log"))
		psl::utility::platform::file::write(psl::utility::application::path::get_path() + sub_path + "main.log", "");
	std::vector<spdlog::sink_ptr> sinks;

	auto mainlogger = std::make_shared<spdlog::sinks::dist_sink_mt>();
	mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "main.log", true));
	mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + "logs/latest.log", true));
	#ifdef _MSC_VER
	mainlogger->add_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>());
	#else
	auto outlogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	outlogger->set_level(spdlog::level::level_enum::warn);
	mainlogger->add_sink(outlogger);
	#endif

	auto ivklogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "ivk.log", true);

	auto igleslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "igles.log", true);

	auto iwgpulogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "iwgpu.log", true);

	auto gfxlogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "gfx.log", true);

	auto systemslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "systems.log", true);

	auto oslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "os.log", true);

	auto datalogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "data.log", true);

	auto corelogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "core.log", true);

	sinks.push_back(mainlogger);
	sinks.push_back(corelogger);

	auto logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
	spdlog::register_logger(logger);
	core::log = logger;


	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(systemslogger);

	auto system_logger = std::make_shared<spdlog::logger>("systems", begin(sinks), end(sinks));
	spdlog::register_logger(system_logger);
	core::systems::log = system_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(oslogger);

	auto os_logger = std::make_shared<spdlog::logger>("os", begin(sinks), end(sinks));
	spdlog::register_logger(os_logger);
	core::os::log = os_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(datalogger);

	auto data_logger = std::make_shared<spdlog::logger>("data", begin(sinks), end(sinks));
	spdlog::register_logger(data_logger);
	core::data::log = data_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(gfxlogger);

	auto gfx_logger = std::make_shared<spdlog::logger>("gfx", begin(sinks), end(sinks));
	spdlog::register_logger(gfx_logger);
	core::gfx::log = gfx_logger;

	#ifdef PE_VULKAN
	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(ivklogger);

	auto ivk_logger = std::make_shared<spdlog::logger>("ivk", begin(sinks), end(sinks));
	spdlog::register_logger(ivk_logger);
	core::ivk::log = ivk_logger;
	#endif
	#ifdef PE_GLES
	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(igleslogger);

	auto igles_logger = std::make_shared<spdlog::logger>("igles", begin(sinks), end(sinks));
	spdlog::register_logger(igles_logger);
	core::igles::log = igles_logger;
	#endif
	#ifdef PE_WEBGPU
	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(iwgpulogger);

	auto iwgpu_logger = std::make_shared<spdlog::logger>("iwgpu", begin(sinks), end(sinks));
	spdlog::register_logger(iwgpu_logger);
	core::iwgpu::log = iwgpu_logger;
	#endif
	spdlog::set_pattern("%8T.%6f [%=8n] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}
#else

	#include "spdlog/sinks/android_sink.h"
auto core::initialize_loggers() -> bool {
	if(core::_loggers_initialized)
		return true;

	core::_loggers_initialized = true;
	core::log				   = spdlog::android_logger_mt("main", "paradigm");
	core::systems::log		   = spdlog::android_logger_mt("systems", "paradigm");
	core::os::log			   = spdlog::android_logger_mt("os", "paradigm");
	core::data::log			   = spdlog::android_logger_mt("data", "paradigm");
	core::gfx::log			   = spdlog::android_logger_mt("gfx", "paradigm");
	#if defined(PE_VULKAN)
	core::ivk::log			   = spdlog::android_logger_mt("ivk", "paradigm");
	#endif
	#if defined(PE_GLES)
	core::igles::log		   = spdlog::android_logger_mt("igles", "paradigm");
	#endif
	#if defined(PE_WEBGPU)
	core::iwgpu::log		   = spdlog::android_logger_mt("iwgpu", "paradigm");
	#endif
	spdlog::set_pattern("[%8T:%6f] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}

#endif
