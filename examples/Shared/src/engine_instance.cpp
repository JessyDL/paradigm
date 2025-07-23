#include "examples/engine_instance.hpp"

#include "core/data/buffer.hpp"
#include "core/data/material.hpp"
#include "core/data/window.hpp"
#include "core/ecs/systems/gpu_camera.hpp"
#include "core/gfx/buffer.hpp"
#include "core/gfx/context.hpp"
#include "core/gfx/material.hpp"
#include "core/gfx/render_graph.hpp"
#include "core/gfx/swapchain.hpp"
#include "core/logging.hpp"
#include "core/os/context.hpp"
#include "core/os/surface.hpp"
#include "core/paradigm.hpp"
#include "psl/application_utils.hpp"
#include "psl/library.hpp"
#include "psl/memory/region.hpp"
#include "psl/platform_utils.hpp"

engine_instance_t::engine_instance_t(options_t options, std::unique_ptr<core::os::context> os_context)
	: m_Backend(options.backend), m_OSContext(std::move(os_context)) {
	core::log->info("Starting the application");
	core::log->info("creating a '{}' backend", core::gfx::graphics_backend_str(m_Backend));
	core::log->info("creating cache");

	auto const format_string = [&](psl::string8_t const& str, psl::string8_t application_name = "") {
		return fmt::format(fmt::runtime(str),
						   fmt::arg("ENGINE_NAME", APPLICATION_NAME),
						   fmt::arg("ENGINE_FULL_NAME", APPLICATION_FULL_NAME),
						   fmt::arg("APPLICATION_NAME", application_name),
						   fmt::arg("GRAPHICS_BACKEND", core::gfx::graphics_backend_str(m_Backend)));
	};

	options.application_name = format_string(options.application_name);
	options.window_title	 = format_string(options.window_title, options.application_name);

	psl::string8_t environment = core::gfx::graphics_backend_str(m_Backend);

	m_MemoryRegion = std::make_unique<memory::region>(options.cpu_backed_memory_region.size,
													  options.cpu_backed_memory_region.alignment,
													  new memory::default_allocator());
	m_Cache = std::make_unique<core::resource::cache_t>(psl::meta::library {"resources.metalib", {{environment}}});

	auto& cache = *m_Cache;

	auto window_data = cache.create<core::data::window>();
	window_data->name(options.window_title);
	m_SurfaceHandle = cache.create<core::os::surface>(window_data);
	if(!m_SurfaceHandle) {
		core::log->critical("Could not create a OS surface to draw on.");
		std::abort();
	}

	m_ContextHandle =
	  cache.create<core::gfx::context>(m_Backend, psl::string8_t {options.application_name}, m_SurfaceHandle);
	auto swapchain_handle = cache.create<core::gfx::swapchain>(m_SurfaceHandle, m_ContextHandle, *m_OSContext);

	m_RenderGraph	  = std::make_unique<core::gfx::render_graph>();
	auto& renderGraph = *m_RenderGraph;
	m_Swapchain		  = renderGraph.create_drawpass(m_ContextHandle, swapchain_handle);

	{
		core::log->info("Creating a staging buffer");
		if(m_Backend == core::gfx::graphics_backend::vulkan) {
			auto stagingBufferData = cache.create<core::data::buffer_t>(
			  core::gfx::memory_usage::transfer_source,
			  core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
			  memory::region {
				options.staging_buffer.size, options.staging_buffer.alignment, new memory::default_allocator(false)});
			m_StagingBufferHandle = cache.create<core::gfx::buffer_t>(m_ContextHandle, stagingBufferData);
		}
		core::log->info("Staging buffer created");
	}
	auto vertexBufferData = cache.create<core::data::buffer_t>(
	  core::gfx::memory_usage::vertex_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {
		options.vertex_buffer.size, options.vertex_buffer.alignment, new memory::default_allocator(false)});
	m_VertexBufferHandle = cache.create<core::gfx::buffer_t>(m_ContextHandle, vertexBufferData, m_StagingBufferHandle);

	auto indexBufferData = cache.create<core::data::buffer_t>(
	  core::gfx::memory_usage::index_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {options.index_buffer.size, options.index_buffer.alignment, new memory::default_allocator(false)});
	m_IndexBufferHandle = cache.create<core::gfx::buffer_t>(m_ContextHandle, indexBufferData, m_StagingBufferHandle);

	auto const uniform_buffer_align = m_ContextHandle->limits().uniform.alignment;

	auto instanceBufferData = cache.create<core::data::buffer_t>(
	  core::gfx::memory_usage::vertex_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {
		options.instance_buffer.size, options.instance_buffer.alignment, new memory::default_allocator(false)});
	m_InstanceBufferHandle =
	  cache.create<core::gfx::buffer_t>(m_ContextHandle, instanceBufferData, m_StagingBufferHandle);

	auto instanceMaterialBufferData = cache.create<core::data::buffer_t>(
	  core::gfx::memory_usage::uniform_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {
		options.instance_material_buffer.size, uniform_buffer_align, new memory::default_allocator(false)});
	m_InstanceMaterialBufferHandle =
	  cache.create<core::gfx::buffer_t>(m_ContextHandle, instanceMaterialBufferData, m_StagingBufferHandle);
	m_InstanceMaterialBindingHandle = cache.create<core::gfx::shader_buffer_binding>(
	  m_InstanceMaterialBufferHandle, options.instance_material_binding.size);
	cache.library().set(m_InstanceMaterialBindingHandle.uid(), core::data::material_t::MATERIAL_DATA);

	auto globalShaderBufferData = cache.create<core::data::buffer_t>(
	  core::gfx::memory_usage::uniform_buffer,
	  core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
	  m_MemoryRegion
		->create_region(options.global_shader_buffer.size, uniform_buffer_align, new memory::default_allocator(true))
		.value());

	m_GlobalShaderBufferHandle = cache.create<core::gfx::buffer_t>(m_ContextHandle, globalShaderBufferData);
	m_FrameCamBufferBindingHandle =
	  cache.create<core::gfx::shader_buffer_binding>(m_GlobalShaderBufferHandle,
													 options.frame_cam_buffer_binding.size,
													 sizeof(core::ecs::systems::gpu_camera::framedata));
	cache.library().set(m_FrameCamBufferBindingHandle, "GLOBAL_DYNAMIC_WORLD_VIEW_PROJECTION_MATRIX");
}

engine_instance_t::~engine_instance_t() {
	// we clear these so no dangling references exist when we destroy the cache.
	// the order doesn't matter as the resources will persist till the cache itself
	// tries to clean them up.
	// In a real application it would be recommended to not store handles in the same scope as the cache.
	m_FrameCamBufferBindingHandle	= {};
	m_InstanceMaterialBindingHandle = {};
	m_GlobalShaderBufferHandle		= {};
	m_InstanceMaterialBufferHandle	= {};
	m_InstanceBufferHandle			= {};
	m_IndexBufferHandle				= {};
	m_VertexBufferHandle			= {};
	m_ContextHandle					= {};
	m_SurfaceHandle					= {};

	m_RenderGraph.reset();
	m_Cache.reset();
	m_MemoryRegion.reset();
}

void engine_instance_t::run(std::function<void(engine_instance_t const&, std::chrono::duration<float>)> callback) {
	std::chrono::high_resolution_clock::time_point last_tick = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> dTime {};
	std::chrono::duration<float> elapsed {};

	while(m_OSContext->tick() && m_SurfaceHandle->tick()) {
		auto current_time = std::chrono::high_resolution_clock::now();
		dTime			  = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_tick);
		elapsed += dTime;
		last_tick = current_time;
		callback(*this, dTime);

		m_RenderGraph->present();
	}
}

core::resource::cache_t& engine_instance_t::cache() {
	psl_assert(m_Cache, "Cache is not initialized");
	return *m_Cache;
}
