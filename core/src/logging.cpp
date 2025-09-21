#include "core/logging.hpp"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#ifdef _MSC_VER
	#include "spdlog/sinks/msvc_sink.h"
#endif

#include "psl/assertions.hpp"
#include <iostream>

/// \brief A custom streambuf that intercepts the std::cout and redirects it to the spdlog logger
/// \details This streambuf will capture the output from std::cout and parse it for special control characters
/// redirecting it to the corresponding log level in the `core` sink.
/// \todo This could be improved to additionally parse the file location (if present) to redirect it to other sinks
class spdlogbuf : public std::streambuf {
	void log_buffer() {
		std::string_view view {buffer_};
		if(psl_log) {
			size_t start = view.find(']');
			assert(start != std::string_view::npos && start > 1);
			start	   = view.find_first_not_of(' ', start + 1);	// first non-space character after the ']'
			size_t end = view.size() - (2 + start);					// 2 control characters (including the endline)
			view	   = view.substr(start, end);
			switch(level_) {
			case psl::level_t::info:
				core::log->info("{}", view);
				break;
			case psl::level_t::warn:
				core::log->warn("{}", view);
				break;
			case psl::level_t::error:
				core::log->error("{}", view);
				break;
			case psl::level_t::debug:
				core::log->debug("{}", view);
				break;
			case psl::level_t::verbose:
				core::log->trace("{}", view);
				break;
			case psl::level_t::fatal:
				core::log->critical("{}", view);
				break;
			default:
				core::log->info("{}", view);
				break;
			}
		} else {
			core::log->info("{}", view);
		}
		buffer_.clear();
		psl_log = false;
	}

  public:
	int_type overflow(int_type ch) override {
		if(ch != '\x1F') {
			oldbuf_->sputc(ch);
		}
		if(ch != EOF && (ch != '\n' || psl_log)) {
			buffer_ += static_cast<char>(ch);
		}
		if(ch == '\n') {
			std::string_view view {buffer_};
			if(psl_log) {
				if(view.ends_with("\x1F\n")) {
					log_buffer();
				}
			} else if(view.starts_with("\x1F[")) {
				view	   = view.substr(2);	// remove the first '\x1F['
				psl_log	   = true;
				size_t pos = view.find(']');
				if(view.starts_with("info]")) {
					level_ = psl::level_t::info;
				} else if(view.starts_with("warn]")) {
					level_ = psl::level_t::warn;
				} else if(view.starts_with("error]")) {
					level_ = psl::level_t::error;
				} else if(view.starts_with("debug]")) {
					level_ = psl::level_t::debug;
				} else if(view.starts_with("verbose]")) {
					level_ = psl::level_t::verbose;
				} else if(view.starts_with("fatal]")) {
					level_ = psl::level_t::fatal;
				} else {
					psl_log = false;
				}

				if(psl_log) {
					buffer_ += '\n';
				}

				if(view.ends_with("\x1F") && psl_log) {
					log_buffer();
				}
			} else {
				log_buffer();
			}
		}
		return ch;
	}

  private:
	psl::level_t level_ = psl::level_t::info;
	bool psl_log		= false;	// true if we are logging through psl::print

	std::string buffer_;
	std::streambuf* oldbuf_ = std::cout.rdbuf();
};

static spdlogbuf sbuf;

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

auto core::initialize_loggers(bool to_file, bool to_terminal) -> void {
	if(core::_loggers_initialized) {
		return;
	}

	core::_loggers_initialized = true;

	std::ostream out(&sbuf);
	std::cout.rdbuf(&sbuf);

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
	if(to_terminal) {
	#ifdef _MSC_VER
		mainlogger->add_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>());
	#else
		auto outlogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		outlogger->set_level(spdlog::level::level_enum::warn);
		mainlogger->add_sink(outlogger);
	#endif
	}

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
	spdlog::flush_on(spdlog::level::warn);
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
