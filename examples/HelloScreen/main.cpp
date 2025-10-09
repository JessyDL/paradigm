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

// This is the entry point for our agnostic renderer. The following code works the same for all platforms and graphics
// backends.
// note for android you will need to look at the android example, as it requires a different entry point.
int entry(core::gfx::graphics_backend backend, core::os::context& os_context) {
	core::log->info("Starting the application");
	core::log->info("creating a '{}' backend", core::gfx::graphics_backend_str(backend));

	// Here we initialize the cache. The cache is a central place where all resources are stored.
	// The cache internally stores handles to resources, this means if resources go out of scope
	// the cache will automatically clean up the resources if requested.
	// Additionally resources can store references to one another, to protect you from deleting
	// resources that are still in use, and to see where leaks are coming from.
	//
	// Normally we'd load a library from a file, but in this case we're using a empty library.
	core::log->info("creating cache");
	core::resource::cache_t cache {psl::meta::library {}};
	core::log->info("cache created");

	// We create a window data object. This object contains information about the window, such as its name, width,
	// height, etc.. This object is used to create a surface, which is provided by your OS. Most objects have direct
	// counterpoles in the core::data namespace. Such as textures. This namespace contains all the data that is used to
	// create resources, or store them. They are the descriptors for the resources.
	auto window_data = cache.create<core::data::window>();
	window_data->name(APPLICATION_FULL_NAME + " { " + core::gfx::graphics_backend_str(backend) + " }");
	auto surface_handle = cache.create<core::os::surface>(window_data);
	if(!surface_handle) {
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	// Next up we initialize the graphics context. This is the object that is used to create the graphics backend.
	// The graphics context is used to create the swapchain, which is the object that is used to present the rendered
	// images to the screen.
	auto context_handle = cache.create<core::gfx::context>(backend, psl::string8_t {APPLICATION_NAME}, surface_handle);
	auto swapchain_handle = cache.create<core::gfx::swapchain>(surface_handle, context_handle, os_context);

	// We set the clear color of the swapchain. This is the color that is used to clear the screen before rendering.
	// in this example's case this means the screen will be fully magenta.
	swapchain_handle->clear_color({1.0f, 0.0f, 1.0f, 1.0f});

	// All rendering internally is done through a graph, this helps in tracking when and where resources are used and
	// how they are used. So we know where we need to enter the sync points in the rendering. For now it's just
	// important to know that the render graph consists out of N draw/compute passes, and those passes contain the
	// actual objects you wish to render (such as meshes, textures, etc..).
	core::gfx::render_graph renderGraph {};

	// We create a drawpass for the swapchain. This drawpass is used to render to the swapchain.
	auto swapchain_pass = renderGraph.create_drawpass(context_handle, swapchain_handle);

	while(os_context.tick() && surface_handle->tick()) {
		// Finally we present, this will kick off all drawpasses in the render graph, and present the final image to the
		// screen.
		renderGraph.present();
	}
	return 0;
}

int main(int argc, char** argv) {
	// We start off by initializing the loggers. Various member and free functions use these loggers to report their
	// status. If you wish to use your own loggers, you can replace the initialize_loggers function with your own
	// implementation.
	core::initialize_loggers();

	// This decodes the flags that can be passed to the application. The flags are used to determine the graphics
	// backend.
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

	// We create a context object that will be used to manage the OS context.
	// In many cases this is empty, but in some cases it contains vital information.
	// See 'core/os/context_android.cpp' as example.
	core::os::context context {};
	return entry(backend, context);
}
