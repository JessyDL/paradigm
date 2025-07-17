#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "psl/application_utils.hpp"
#include "psl/library.hpp"
#include "psl/platform_utils.hpp"

#include "core/data/window.hpp"
#include "core/gfx/context.hpp"
#include "core/gfx/render_graph.hpp"
#include "core/gfx/swapchain.hpp"
#include "core/logging.hpp"
#include "core/os/context.hpp"
#include "core/os/surface.hpp"
#include "core/paradigm.hpp"
#include "core/resource/resource.hpp"

#include "core/bindings/csharp/runtime.hpp"
#include "psl/ecs/state.hpp"

int entry(core::gfx::graphics_backend backend, core::os::context& os_context) {
	core::log->info("Starting the application");
	core::log->info("creating a '{}' backend", core::gfx::graphics_backend_str(backend));

	core::log->info("creating cache");
	core::resource::cache_t cache {psl::meta::library {}};
	core::log->info("cache created");

	auto runtime = core::bindings::csharp::runtime {psl::utility::application::path::project, "csharp_bindings"};
	assert(runtime.is_running());

	runtime.unsafe_invoke("Paradigm.Example", "Initialize", [](auto& fn) {
		fn();
		core::log->info("ECS System Description:");
	});

	runtime.unsafe_invoke<psl::string>("Paradigm.Example", "HelloWorldReturn", [](auto& fn) {
		auto result = fn();
		core::log->info("ECS System Description: {}", result);
	});
	runtime.unsafe_invoke<psl::string>("Paradigm.Example", "GetAllEcsSystemMethodsString", [](auto& fn) {
		auto result = fn();
		core::log->info("ECS System Description: {}", result);
	});

	auto window_data = cache.create<core::data::window>();
	window_data->name(APPLICATION_FULL_NAME + " { " + core::gfx::graphics_backend_str(backend) + " }");
	auto surface_handle = cache.create<core::os::surface>(window_data);
	if(!surface_handle) {
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	auto context_handle = cache.create<core::gfx::context>(backend, psl::string8_t {APPLICATION_NAME}, surface_handle);
	auto swapchain_handle = cache.create<core::gfx::swapchain>(surface_handle, context_handle, os_context);

	swapchain_handle->clear_color({1.0f, 0.0f, 1.0f, 1.0f});

	core::gfx::render_graph renderGraph {};

	auto swapchain_pass = renderGraph.create_drawpass(context_handle, swapchain_handle);

	psl::ecs::state_t ECSState {};


	while(os_context.tick() && surface_handle->tick()) {
		renderGraph.present();
	}
	return 0;
}

int main(int argc, char** argv) {
	core::initialize_loggers();

	if(argc > 0) {
		core::log->info("Received the cli args:");
		for(auto i = 0; i < argc; ++i) core::log->info(argv[i]);
	}
	auto backend = [](int argc, char* argv[]) {
		for(auto i = 0; i < argc; ++i) {
			std::string_view text {argv[i]};
			if(text == "--vulkan") {
#if defined(PE_VULKAN)
				return core::gfx::graphics_backend::vulkan;
#else
				throw std::runtime_error("Requested a Vulkan backend, but application does not support Vulkan");
#endif
			} else if(text == "--gles") {
#if defined(PE_GLES)
				return core::gfx::graphics_backend::gles;
#else
				throw std::runtime_error("Requested a GLES backend, but application does not support GLES");
#endif
			} else if(text == "--webgpu") {
#if defined(PE_WEBGPU)
				return core::gfx::graphics_backend::webgpu;
#else
				throw std::runtime_error("Requested a WebGPU backend, but application does not support WebGPU");
#endif
			}
		}
#if defined(PE_VULKAN)
		return core::gfx::graphics_backend::vulkan;
#elif defined(PE_WEBGPU)
		return core::gfx::graphics_backend::webgpu;
#elif defined(PE_GLES)
		return core::gfx::graphics_backend::gles;
#endif
		return core::gfx::graphics_backend::undefined;
	}(argc, argv);

	core::os::context context {};
	return entry(backend, context);
}
