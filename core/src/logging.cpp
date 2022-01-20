
#include "logging.hpp"


std::shared_ptr<spdlog::logger> core::log {nullptr};
std::shared_ptr<spdlog::logger> core::gfx::log {nullptr};
#ifdef PE_VULKAN
std::shared_ptr<spdlog::logger> core::ivk::log {nullptr};
#endif
#ifdef PE_GLES
std::shared_ptr<spdlog::logger> core::igles::log {nullptr};
#endif
std::shared_ptr<spdlog::logger> core::data::log {nullptr};
std::shared_ptr<spdlog::logger> core::systems::log {nullptr};
std::shared_ptr<spdlog::logger> core::os::log {nullptr};


psl::profiling::profiler core::profiler {};