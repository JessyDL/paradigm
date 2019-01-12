
#include "logging.h"


std::shared_ptr<spdlog::logger> core::log{nullptr};
std::shared_ptr<spdlog::logger> core::gfx::log{nullptr};
std::shared_ptr<spdlog::logger> core::ivk::log{nullptr};
std::shared_ptr<spdlog::logger> core::data::log{nullptr};
std::shared_ptr<spdlog::logger> core::systems::log{nullptr};
std::shared_ptr<spdlog::logger> core::os::log{nullptr};


psl::profiling::profiler core::profiler{};