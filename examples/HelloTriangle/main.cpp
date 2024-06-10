

// In this example we will create a simple triangle on the screen.
// A lot of the initialization code is no longer documented, if you wish to understand the initialization process
// better, please refer to the 'HelloScreen' example.

// See the #region tags for the relevant code snippets, they will be marked with `example`


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

#include "core/data/buffer.hpp"
#include "core/data/geometry.hpp"
#include "core/gfx/buffer.hpp"
#include "core/gfx/geometry.hpp"
#include "core/gfx/types.hpp"
#include "psl/memory/region.hpp"


// The staging is _entirely_ optional, but it's a good way to get data to the GPU.
// note that if you only intend to target gles, you can skip the staging buffer.
auto create_staging(auto backend, auto& cache, auto& context_handle) -> core::resource::handle<core::gfx::buffer_t> {
	core::log->info("Creating a staging buffer");
	core::resource::handle<core::gfx::buffer_t> stagingBuffer {};

	// we only need a staging buffer for vulkan
	// todo: we should probably create a helper function for this, a queryable function that returns features that are
	// supported
	if(backend == core::gfx::graphics_backend::vulkan) {
		auto stagingBufferData = cache.create<core::data::buffer_t>(
		  core::gfx::memory_usage::transfer_source,
		  core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
		  memory::region {(size_t)128_mb, 4, new memory::default_allocator(false)});
		stagingBuffer = cache.create<core::gfx::buffer_t>(context_handle, stagingBufferData);
	}
	core::log->info("Staging buffer created");
	return stagingBuffer;
}

int entry(core::gfx::graphics_backend backend, core::os::context& os_context) {
	core::log->info("Starting the application");
	core::log->info("creating a '{}' backend", core::gfx::graphics_backend_str(backend));
	core::log->info("creating cache");
	core::resource::cache_t cache {psl::meta::library {}};
	core::log->info("cache created");
	auto window_data = cache.create<core::data::window>();
	window_data->name(APPLICATION_FULL_NAME + " { " + core::gfx::graphics_backend_str(backend) + " }");
	auto surface_handle = cache.create<core::os::surface>(window_data);
	if(!surface_handle) {
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	auto context_handle = cache.create<core::gfx::context>(backend, psl::string8_t {APPLICATION_NAME}, surface_handle);
	auto swapchain_handle = cache.create<core::gfx::swapchain>(surface_handle, context_handle, os_context);

	core::gfx::render_graph renderGraph {};
	auto swapchain_pass = renderGraph.create_drawpass(context_handle, swapchain_handle);

#pragma region example
	// First we start by creating a staging buffer. It is unneeded for the workload of this example, but it's a good
	// practice to use a staging buffer to get data to the GPU.
	// The idea behind a staging buffer is to create a buffer that is optimized for CPU access, and then copy the data
	// from the staging buffer to the GPU buffer.
	// On some graphics hardware this can be skipped if they support shared memory architecture.

	core::resource::handle<core::gfx::buffer_t> stagingBuffer {};
	{
		// The staging is _entirely_ optional, but it's a good way to get data to the GPU.
		// note that if you only intend to target gles, you can skip the staging buffer.
		core::log->info("Creating a staging buffer");

		// we only need a staging buffer for vulkan
		// todo: we should probably create a helper function for this, a queryable function that returns features that
		// are supported
		if(backend == core::gfx::graphics_backend::vulkan) {
			auto stagingBufferData = cache.create<core::data::buffer_t>(
			  core::gfx::memory_usage::transfer_source,
			  core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
			  memory::region {(size_t)32_mb, 4, new memory::default_allocator(false)});
			stagingBuffer = cache.create<core::gfx::buffer_t>(context_handle, stagingBufferData);
		}
		core::log->info("Staging buffer created");
	}

	// next up we will create the vertex and index buffers to store our mesh data in.
	// special note is that the memory::region it's allocator is not physically backed, which means
	// it is solely used to determine vram usage. If you want mesh data to be CPU accessible, you should
	// make the memory::region backed by a physical allocator.

	// unlike the staging buffer, the vertex and index buffers will be `device_local`, meaning they are only accessible
	// by the GPU. This is the most performant way to store data on the GPU, but if you need to frequently update the
	// data, or need to read the data back, you should look at other property flags.

	// similarly the usage flags is a hint to the GPU on how the data will be used. This is important for usage features
	// and performance. If you use the wrong flags, the GPU will not be able to optimize the data properly for the
	// intended use.
	auto vertexBufferData = cache.create<core::data::buffer_t>(
	  core::gfx::memory_usage::vertex_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {32_mb, 4, new memory::default_allocator(false)});
	auto vertexBuffer = cache.create<core::gfx::buffer_t>(context_handle, vertexBufferData, stagingBuffer);

	auto indexBufferData = cache.create<core::data::buffer_t>(
	  core::gfx::memory_usage::index_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {32_mb, 4, new memory::default_allocator(false)});
	auto indexBuffer = cache.create<core::gfx::buffer_t>(context_handle, indexBufferData, stagingBuffer);

	// next up we will create the geometry data. This data will be uploaded to the GPU and used to render the triangle.
	// this is the equivalent of a "model", but in this case we will construct it through code.
	auto triangleGeomData = cache.create<core::data::geometry_t>();
	{
		core::vertex_stream_t vertexStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t colorStream {core::vertex_stream_t::type::vec3};

		auto& vertices = vertexStream.get<core::vertex_stream_t::type::vec3>();
		auto& colors   = colorStream.get<core::vertex_stream_t::type::vec3>();

		vertices.emplace_back(psl::vec3 {-0.5f, 0.0f, 0.0f});
		vertices.emplace_back(psl::vec3 {0.5f, 0.0f, 0.0f});
		vertices.emplace_back(psl::vec3 {0.0f, 0.5f, 0.0f});

		colors.emplace_back(psl::vec3 {1.0f, 0.0f, 0.0f});
		colors.emplace_back(psl::vec3 {0.0f, 1.0f, 0.0f});
		colors.emplace_back(psl::vec3 {0.0f, 0.0f, 1.0f});

		triangleGeomData->vertices(core::data::geometry_t::constants::POSITION, vertexStream);
		triangleGeomData->vertices(core::data::geometry_t::constants::COLOR, colorStream);

		triangleGeomData->indices(std::vector<uint32_t> {0, 1, 2});
	}
	// now with the geometry data container, we can create a gfx resource. This is the object that will be responsible
	// for synchronizing the data between the CPU and GPU.
	// together with a material you can use this resource to render the data.

	// in general objects in core::data namespace are the RAM backed resources, and mostly used to create the GPU
	// resources in the core::gfx namespace.
	auto triangleGeometryResource =
	  cache.create<core::gfx::geometry_t>(context_handle, triangleGeomData, vertexBuffer, indexBuffer);
#pragma endregion example

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
