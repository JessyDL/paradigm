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

auto core::initialize_loggers(bool to_file) -> void {
	if(core::_loggers_initialized) {
		return;
	}

	core::_loggers_initialized = true;

	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c						  = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm							  = localtime_safe(now_c);
	psl::string time;
	time.resize(20);
	strftime(time.data(), 20, "%Y-%m-%d %H-%M-%S", &now_tm);
	time[time.size() - 1] = '/';
	auto path			  = psl::utility::application::path::get_path();
	psl::string sub_path  = path + "logs/" + time;
	std::vector<spdlog::sink_ptr> sinks;

	auto make_sink = [](std::shared_ptr<spdlog::logger>& target,
						psl::string_view name,
						std::optional<psl::string_view> path,
						auto&... additional_sinks) {
		std::vector<spdlog::sink_ptr> sinks;
		sinks.reserve(sizeof...(additional_sinks) + 1);
		if(path) {
			sinks.emplace_back(
			  std::make_shared<spdlog::sinks::basic_file_sink_mt>(psl::string {path.value()} + name + ".log", true));
		}
		(sinks.emplace_back(additional_sinks), ...);
		auto logger = std::make_shared<spdlog::logger>(psl::string {name}, begin(sinks), end(sinks));
		spdlog::register_logger(logger);
		target = logger;
	};

	auto mainlogger = std::make_shared<spdlog::sinks::dist_sink_mt>();
	if(to_file) {
		mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(sub_path + "main.log", true));
		mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path + "logs/latest.log", true));
	}
	#ifdef _MSC_VER
	mainlogger->add_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>());
	#else
	auto outlogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	outlogger->set_level(spdlog::level::level_enum::warn);
	mainlogger->add_sink(outlogger);
	#endif

	make_sink(core::log, "core", to_file ? std::make_optional(sub_path) : std::nullopt, mainlogger);
	make_sink(core::gfx::log, "gfx", to_file ? std::make_optional(sub_path) : std::nullopt, mainlogger);
	#if defined(PE_VULKAN)
	make_sink(core::ivk::log, "ivk", to_file ? std::make_optional(sub_path) : std::nullopt, mainlogger);
	#endif
	#if defined(PE_GLES)
	make_sink(core::igles::log, "igles", to_file ? std::make_optional(sub_path) : std::nullopt, mainlogger);
	#endif
	#if defined(PE_WEBGPU)
	make_sink(core::iwgpu::log, "iwgpu", to_file ? std::make_optional(sub_path) : std::nullopt, mainlogger);
	#endif
	make_sink(core::data::log, "data", to_file ? std::make_optional(sub_path) : std::nullopt, mainlogger);
	make_sink(core::systems::log, "systems", to_file ? std::make_optional(sub_path) : std::nullopt, mainlogger);
	make_sink(core::os::log, "os", to_file ? std::make_optional(sub_path) : std::nullopt, mainlogger);

	spdlog::set_pattern("%8T.%6f [%=8n] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}
#else

	#include "spdlog/sinks/android_sink.h"
auto core::initialize_loggers([[maybe_unused]] bool to_file) -> void {
	if(core::_loggers_initialized) {
		return;
	}

	core::_loggers_initialized = true;
	core::log				   = spdlog::android_logger_mt("core", "paradigm");
	core::systems::log		   = spdlog::android_logger_mt("systems", "paradigm");
	core::os::log			   = spdlog::android_logger_mt("os", "paradigm");
	core::data::log			   = spdlog::android_logger_mt("data", "paradigm");
	core::gfx::log			   = spdlog::android_logger_mt("gfx", "paradigm");
	#if defined(PE_VULKAN)
	core::ivk::log = spdlog::android_logger_mt("ivk", "paradigm");
	#endif
	#if defined(PE_GLES)
	core::igles::log = spdlog::android_logger_mt("igles", "paradigm");
	#endif
	#if defined(PE_WEBGPU)
	core::iwgpu::log = spdlog::android_logger_mt("iwgpu", "paradigm");
	#endif
	spdlog::set_pattern("[%8T:%6f] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}

#endif
