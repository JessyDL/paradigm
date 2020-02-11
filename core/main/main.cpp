
// core.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS
//#include <Windows.h>
//#include "stdafx.h"
#include "psl/library.h"
#include "psl/application_utils.h"
#include "resource/resource.hpp"

#include "paradigm.hpp"

#include "logging.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#ifdef _MSC_VER
#include "spdlog/sinks/msvc_sink.h"
#endif
#include "gfx/limits.h"
#include "gfx/types.h"
#include "utility/geometry.h"

#include "data/window.h" // application data
#include "data/buffer.h"
#include "data/geometry.h"
#include "data/sampler.h"
#include "data/material.h"

#include "os/surface.h" // the OS surface to draw one

#include "meta/shader.h"
#include "meta/texture.h"

#include "gfx/context.h"
#include "gfx/buffer.h"
#include "gfx/geometry.h"
#include "gfx/texture.h"
#include "gfx/swapchain.h"
#include "gfx/sampler.h"
#include "gfx/pipeline_cache.h"
#include "gfx/material.h"
#include "gfx/framebuffer.h"
#include "gfx/shader.h"
#include "gfx/compute.h"

#include "gfx/bundle.h"
#include "gfx/render_graph.h"
#include "gfx/computecall.h"
#include "gfx/drawpass.h"
#include "gfx/computepass.h"

#include "psl/ecs/state.h"
#include "ecs/components/transform.h"
#include "ecs/components/camera.h"
#include "ecs/components/input_tag.h"
#include "ecs/components/renderable.h"
#include "ecs/components/lifetime.h"
#include "ecs/components/dead_tag.h"
#include "ecs/components/velocity.h"

#include "ecs/systems/gpu_camera.h"
#include "ecs/systems/render.h"
#include "ecs/systems/fly.h"
#include "ecs/systems/geometry_instance.h"
#include "ecs/systems/lifetime.h"
#include "ecs/systems/death.h"
#include "ecs/systems/attractor.h"
#include "ecs/systems/movement.h"
#include "ecs/systems/lighting.h"
#include "ecs/systems/text.h"

#include "psl/noise/perlin.h"
#include "ecs/systems/debug/grid.h"

using namespace core;
using namespace core::resource;
using namespace core::gfx;

using namespace psl::ecs;
using namespace core::ecs::components;

handle<core::gfx::compute> create_compute(resource::cache& cache, handle<core::gfx::context> context_handle,
										  handle<core::gfx::pipeline_cache> pipeline_cache, const psl::UID& shader,
										  const psl::UID& texture)
{
	auto meta = cache.library().get<core::meta::shader>(shader).value();
	auto data = cache.create<data::material>();
	data->from_shaders(cache.library(), {meta});
	auto stages = data->stages();
	for(auto& stage : stages)
	{
		auto bindings = stage.bindings();
		bindings[0].texture(texture);
		stage.bindings(bindings);
	}
	data->stages(stages);
	return cache.instantiate<core::gfx::compute>("594b2b8a-d4ea-e162-2b2c-987de571c7be"_uid, context_handle, data,
												 pipeline_cache);
}

handle<core::gfx::material> setup_gfx_material(resource::cache& cache, handle<core::gfx::context> context_handle,
											   handle<core::gfx::pipeline_cache> pipeline_cache,
											   handle<core::gfx::buffer> matBuffer, psl::UID vert, psl::UID frag,
											   const psl::UID& texture)
{
	auto vertShaderMeta = cache.library().get<core::meta::shader>(vert).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>(frag).value();
	auto textureHandle	= cache.instantiate<gfx::texture>(texture, context_handle);

	assert(textureHandle);
	// create the sampler
	auto samplerData   = cache.create<data::sampler>();
	auto samplerHandle = cache.create<gfx::sampler>(context_handle, samplerData);

	// load the example material
	auto matData = cache.create<data::material>();

	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});

	auto stages = matData->stages();
	for(auto& stage : stages)
	{
		if(stage.shader_stage() != core::gfx::shader_stage::fragment) continue;

		auto bindings = stage.bindings();
		bindings[0].texture(texture);
		bindings[0].sampler(samplerHandle);
		stage.bindings(bindings);
		// binding.texture()
	}
	matData->stages(stages);
	matData->blend_states({core::data::material::blendstate(0)});
	// matData->wireframe(true);
	auto material = cache.create<core::gfx::material>(context_handle, matData, pipeline_cache, matBuffer);

	return material;
}


handle<core::gfx::material> setup_gfx_depth_material(resource::cache& cache, handle<core::gfx::context> context_handle,
													 handle<core::gfx::pipeline_cache> pipeline_cache,
													 handle<core::gfx::buffer> matBuffer)
{
	auto vertShaderMeta = cache.library().get<core::meta::shader>("ae4990d1-afa1-3443-3780-53ba7e667880"_uid).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>("764e7814-ce91-34ea-1a1f-1cd8d49a82b9"_uid).value();


	auto matData = cache.create<data::material>();

	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});

	auto material = cache.create<gfx::material>(context_handle, matData, pipeline_cache, matBuffer);
	return material;
}

#ifndef PLATFORM_ANDROID
void setup_loggers()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c						  = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm							  = *std::localtime(&now_c);
	psl::string time;
	time.resize(20);
	strftime(time.data(), 20, "%Y-%m-%d %H-%M-%S", &now_tm);
	time[time.size() - 1] = '/';
	psl::string sub_path  = "logs/" + time;
	if(!utility::platform::file::exists(utility::application::path::get_path() + sub_path + "main.log"))
		utility::platform::file::write(utility::application::path::get_path() + sub_path + "main.log", "");
	std::vector<spdlog::sink_ptr> sinks;

	auto mainlogger = std::make_shared<spdlog::sinks::dist_sink_mt>();
	mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "main.log", true));
	mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + "logs/latest.log", true));
#ifdef _MSC_VER
	mainlogger->add_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#else
	auto outlogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	outlogger->set_level(spdlog::level::level_enum::warn);
	mainlogger->add_sink(outlogger);
#endif

	auto ivklogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "ivk.log", true);

	auto igleslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "igles.log", true);

	auto gfxlogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "gfx.log", true);

	auto systemslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "systems.log", true);

	auto oslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "os.log", true);

	auto datalogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "data.log", true);

	auto corelogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "core.log", true);

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
	spdlog::set_pattern("%8T.%6f [%=8n] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}
#else
#include "spdlog/sinks/android_sink.h"
void setup_loggers()
{
	core::log		   = spdlog::android_logger_mt("main");
	core::systems::log = spdlog::android_logger_mt("systems");
	core::os::log	   = spdlog::android_logger_mt("os");
	core::data::log	   = spdlog::android_logger_mt("data");
	core::gfx::log	   = spdlog::android_logger_mt("gfx");
	core::ivk::log	   = spdlog::android_logger_mt("ivk");
	spdlog::set_pattern("[%8T:%6f] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}

#endif

#if defined(MULTI_CONTEXT_RENDERING)
#include <atomic>
#include <shared_mutex>
namespace core::systems
{
	class renderer_view;

	class renderer
	{
	  public:
		explicit renderer(cache* cache) : m_Cache(cache), deviceIndex(0) {}
		~renderer();
		renderer(renderer&& other) = delete;
		renderer& operator=(renderer&& other) = delete;
		renderer(const renderer& other)		  = delete;
		renderer& operator=(const renderer& other) = delete;

		void start() { m_Thread = std::thread(&core::systems::renderer::main, this); }
		void lock() { mutex.lock(); }
		void unlock() { mutex.unlock(); }

		core::systems::renderer_view& create_view(handle<surface> surface);
		cache& get_cache() { return *m_Cache; }

		const std::vector<renderer_view*>& views() { return m_Views; }

		void close(renderer_view* view);

	  private:
		void main();
		std::shared_mutex mutex;
		uint32_t deviceIndex = 0u;
		cache* m_Cache;
		handle<context> m_Context;
		std::thread m_Thread;
		std::atomic<bool> m_ForceClose{false};
		std::atomic<bool> m_Closeable{false};
		std::vector<renderer_view*> m_Views;
	};
	class renderer_view
	{
		friend class renderer;

	  public:
		renderer_view(renderer* renderer, handle<surface> surface, handle<context> context)
			: m_Renderer(renderer), m_Surface(surface), m_Context(context), m_Swapchain(create_swapchain()),
			  m_Pass(m_Context, m_Swapchain)
		{}
		~renderer_view() { m_Surface.unload(true); };
		handle<surface>& current_surface() { return m_Surface; }

	  private:
		handle<swapchain> create_swapchain()
		{
			auto swapchain_handle = create<swapchain>(m_Renderer->get_cache());
			swapchain_handle.load(m_Surface, m_Context);
			return swapchain_handle;
		}
		void present()
		{
			if(m_Surface->open()) m_Pass.present();
		}
		renderer* m_Renderer;
		handle<surface> m_Surface;
		handle<context> m_Context;
		handle<swapchain> m_Swapchain;
		pass m_Pass;
	};

	renderer::~renderer()
	{
		m_ForceClose = true;
		while(!m_Closeable)
		{
		}
		if(m_Thread.joinable())
		{
			m_Thread.join();
		}
		for(auto& view : m_Views) delete(view);
		m_Context.unload();
		if(m_Cache) delete(m_Cache);
	}
	void renderer::main()
	{
		// Utility::OS::RegisterThisThread("RenderThread " + ss.str());
		while(!m_ForceClose)
		{
			mutex.lock();
			{
				for(auto& view : m_Views)
				{
					auto& surf = view->current_surface();
					if(surf->open()) view->present();
				}
			}
			mutex.unlock();
		}

		m_Closeable = true;
	}

	renderer_view& renderer::create_view(handle<surface> surface)
	{
		mutex.lock();
		m_Context = create<context>(*m_Cache);
		if(!m_Context.load(APPLICATION_FULL_NAME, deviceIndex))
		{
			throw std::runtime_error("no vulkan context could be created");
		}

		auto& view = *m_Views.emplace_back(new renderer_view(this, surface, m_Context));
		mutex.unlock();
		return view;
	}

	void renderer::close(renderer_view* view)
	{
		auto it = std::find(std::begin(m_Views), std::end(m_Views), view);
		if(it == std::end(m_Views)) return;
		mutex.lock();
		delete(*it);
		m_Views.erase(it);
		mutex.unlock();
	}
} // namespace core::systems

core::systems::renderer* init(size_t views = 1)
{
	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	cache cache{psl::meta::library{psl::to_string8_t(libraryPath)}, resource_region.allocator()};

	auto window_data = create_shared<data::window>(cache, "cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
	window_data.load();

	auto rend = new core::systems::renderer{&cache};
	for(auto i = 0u; i < views; ++i)
	{
		auto window_handle = create<surface>(cache);
		if(!window_handle.load(window_data))
		{
			// LOG_FATAL << "Could not create a OS surface to draw on.";
			throw std::runtime_error("no OS surface could be created");
		}
		rend->create_view(window_handle);
	}
	rend->start();
	return rend;
}

int main()
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	setup_loggers();
	// Utility::OS::RegisterThisThread("Main");

	std::vector<core::systems::renderer*> renderers;
	{
		renderers.push_back(init(4u));
	}
	uint64_t frameCount = 0u;
	while(renderers.size() > 0)
	{
		for(auto i = 0u; i < renderers.size(); i)
		{
			renderers[i]->lock();
			for(auto& view : renderers[i]->views())
			{
				auto& surf = view->current_surface();
				if(surf->open())
					surf->tick();
				else
				{
					renderers[i]->close(view);
					break;
				}
			}
			if(renderers[i]->views().size() == 0)
			{
				delete(renderers[i]);
				renderers.erase(std::begin(renderers) + i);
			}
			else
			{
				++i;
			}
			renderers[i]->unlock();
		}
		++frameCount;
	}
	return 0;
}
#elif defined(DEDICATED_GRAPHICS_THREAD)

static void render_thread(handle<context> context, handle<swapchain> swapchain, handle<surface> surface, pass* pass)
{
	try
	{
		while(surface->open() && swapchain->is_ready())
		{
			pass->prepare();
			pass->present();
		}
	}
	catch(...)
	{
		core::gfx::log->critical("critical issue happened in rendering thread");
	}
}
int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);

	setup_loggers();

	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	cache cache{psl::meta::library{psl::to_string8_t(libraryPath)}, resource_region.allocator()};

	auto window_data = create<data::window>(cache, "cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
	window_data.load();

	auto window_handle = create<surface>(cache);
	if(!window_handle.load(window_data))
	{
		core::log->critical("could not create a OS surface to draw on.");
		return -1;
	}


	auto context_handle = create<context>(cache);
	if(!context_handle.load(APPLICATION_FULL_NAME))
	{
		core::log->critical("could not create graphics API surface to use for drawing.");
		return -1;
	}

	auto swapchain_handle = create<swapchain>(cache);
	swapchain_handle.load(window_handle, context_handle);
	window_handle->register_swapchain(swapchain_handle);
	pass pass{context_handle, swapchain_handle};

	std::thread tr{&render_thread, context_handle, swapchain_handle, window_handle, &pass};


	uint64_t frameCount = 0u;
	while(window_handle->tick())
	{
		++frameCount;
	}

	tr.join();

	return 0;
}
#else


#if defined(PLATFORM_ANDROID)
bool focused{true};
int android_entry()
{
	setup_loggers();
	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	cache cache{psl::meta::library{psl::to_string8_t(libraryPath)}, resource_region.allocator()};

	auto window_data = create<data::window>(cache, UID::convert("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"));
	window_data.load();

	auto surface_handle = create<surface>(cache);
	if(!surface_handle.load(window_data))
	{
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	auto context_handle = create<context>(cache);
	if(!context_handle.load(APPLICATION_FULL_NAME, 0))
	{
		core::log->critical("Could not create graphics API surface to use for drawing.");
		return -1;
	}

	auto swapchain_handle = create<swapchain>(cache);
	swapchain_handle.load(surface_handle, context_handle);
	surface_handle->register_swapchain(swapchain_handle);
	context_handle->device().waitIdle();
	pass pass{context_handle, swapchain_handle};
	pass.build();
	uint64_t frameCount										 = 0u;
	std::chrono::high_resolution_clock::time_point last_tick = std::chrono::high_resolution_clock::now();

	int ident;
	int events;
	struct android_poll_source* source;
	bool destroy = false;
	focused		 = true;

	while(true)
	{
		while((ident = ALooper_pollAll(focused ? 0 : -1, NULL, &events, (void**)&source)) >= 0)
		{
			if(source != NULL)
			{
				source->process(platform::specifics::android_application, source);
			}
			if(platform::specifics::android_application->destroyRequested != 0)
			{
				destroy = true;
				break;
			}
		}

		if(destroy)
		{
			ANativeActivity_finish(platform::specifics::android_application->activity);
			break;
		}

		if(swapchain_handle->is_ready())
		{
			pass.prepare();
			pass.present();
		}
		auto current_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsed =
			std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_tick);
		last_tick = current_time;
		++frameCount;

		if(frameCount % 60 == 0)
		{
			swapchain_handle->clear_color(vk::ClearColorValue{
				std::array<float, 4>{(float)(std::rand() % 255) / 255.0f, (float)(std::rand() % 255) / 255.0f,
									 (float)(std::rand() % 255) / 255.0f, 1.0f}});
			pass.build();
		}
	}

	return 0;
}


#endif

#if defined(PLATFORM_ANDROID)
static bool initialized = false;

// todo deal with this extern
android_app* platform::specifics::android_application;

void handleAppCommand(android_app* app, int32_t cmd)
{
	switch(cmd)
	{
	case APP_CMD_INIT_WINDOW: initialized = true; break;
	}
}

int32_t handleAppInput(struct android_app* app, AInputEvent* event) {}

void android_main(android_app* application)
{
	application->onAppCmd = handleAppCommand;
	// Screen density
	AConfiguration* config = AConfiguration_new();
	AConfiguration_fromAssetManager(config, application->activity->assetManager);
	// vks::android::screenDensity = AConfiguration_getDensity(config);
	AConfiguration_delete(config);

	while(!initialized)
	{
		int ident;
		int events;
		struct android_poll_source* source;
		bool destroy = false;

		while((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0)
		{
			if(source != NULL)
			{
				source->process(application, source);
			}
		}
	}

	platform::specifics::android_application = application;
	android_entry();
}
#elif defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)

int entry(gfx::graphics_backend backend)
{
	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	psl::string8_t environment = "";
	switch(backend)
	{
	case graphics_backend::gles: environment = "gles"; break;
	case graphics_backend::vulkan: environment = "vulkan"; break;
	}

	cache cache{psl::meta::library{psl::to_string8_t(libraryPath), {{environment}}}};
	// cache cache{psl::meta::library{psl::to_string8_t(libraryPath), {{environment}}}, resource_region.allocator()};

	auto window_data = cache.instantiate<data::window>("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
	window_data->name(APPLICATION_FULL_NAME + " { " + environment + " }");
	auto surface_handle = cache.create<core::os::surface>(window_data);
	if(!surface_handle)
	{
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	auto context_handle = cache.create<core::gfx::context>(backend, psl::string8_t{APPLICATION_NAME});

	auto swapchain_handle = cache.create<core::gfx::swapchain>(surface_handle, context_handle);

	// get a vertex and fragment shader that can be combined, we only need the meta
	if(!cache.library().contains("f889c133-1ec0-44ea-9209-251cd236f887"_uid) ||
	   !cache.library().contains("4429d63a-9867-468f-a03f-cf56fee3c82e"_uid))
	{
		core::log->critical(
			"Could not find the required shader resources in the meta library. Did you forget to copy the files over?");
		if(surface_handle) surface_handle->terminate();
		return -1;
	}

	auto storage_buffer_align = context_handle->limits().storage_buffer_offset_alignment;
	auto uniform_buffer_align = context_handle->limits().uniform_buffer_offset_alignment;

	auto matBufferData = cache.create<core::data::buffer>(
		core::gfx::memory_usage::storage_buffer | core::gfx::memory_usage::transfer_destination,
		core::gfx::memory_property::device_local,
		memory::region{1024 * 1024 * 128, static_cast<uint64_t>(storage_buffer_align),
					   new memory::default_allocator(false)});
	auto matBuffer = cache.create<core::gfx::buffer>(context_handle, matBufferData);

	// create a staging buffer, this is allows for more advantagous resource access for the GPU
	core::resource::handle<gfx::buffer> stagingBuffer{};
	if(backend == graphics_backend::vulkan)
	{
		auto stagingBufferData = cache.create<data::buffer>(
			core::gfx::memory_usage::transfer_source,
			core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
			memory::region{(size_t)1024 * 1024 * 1024, 4, new memory::default_allocator(false)});
		stagingBuffer = cache.create<gfx::buffer>(context_handle, stagingBufferData);
	}

	// create the buffers to store the model in
	// - memory region which we'll use to track the allocations, this is supposed to be virtual as we don't care to
	//   have a copy on the CPU
	// - then we create the vulkan buffer resource to interface with the GPU
	auto vertexBufferData = cache.create<data::buffer>(
		core::gfx::memory_usage::vertex_buffer | core::gfx::memory_usage::transfer_destination,
		core::gfx::memory_property::device_local,
		memory::region{(size_t)1024 * 1024 * 1536, 4, new memory::default_allocator(false)});
	auto vertexBuffer = cache.create<gfx::buffer>(context_handle, vertexBufferData, stagingBuffer);

	auto indexBufferData = cache.create<data::buffer>(
		core::gfx::memory_usage::index_buffer | core::gfx::memory_usage::transfer_destination,
		core::gfx::memory_property::device_local,
		memory::region{1024 * 1024 * 256, 4, new memory::default_allocator(false)});
	auto indexBuffer = cache.create<gfx::buffer>(context_handle, indexBufferData, stagingBuffer);

	auto instanceBufferData = cache.create<data::buffer>(
		core::gfx::memory_usage::vertex_buffer | core::gfx::memory_usage::transfer_destination,
		core::gfx::memory_property::device_local,
		memory::region{(uint64_t)1024 * 1024 * 128, 4, new memory::default_allocator(false)});
	auto instanceBuffer = cache.create<gfx::buffer>(context_handle, instanceBufferData, stagingBuffer);
	std::vector<resource::handle<data::geometry>> geometryDataHandles;
	std::vector<resource::handle<gfx::geometry>> geometryHandles;
	geometryDataHandles.push_back(utility::geometry::create_icosphere(cache, psl::vec3::one, 0));
	geometryDataHandles.push_back(utility::geometry::create_cone(cache, 1.0f, 1.0f, 1.0f, 12));
	geometryDataHandles.push_back(utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	geometryDataHandles.push_back(utility::geometry::create_spherified_cube(cache, psl::vec3::one, 2));
	geometryDataHandles.push_back(utility::geometry::create_box(cache, psl::vec3::one));
	geometryDataHandles.push_back(utility::geometry::create_sphere(cache, psl::vec3::one, 12, 8));
	// up
	geometryDataHandles.push_back(utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::back * 90.0f));
	utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::up * 0.5f);
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::forward * 90.0f),
							  core::data::geometry::constants::NORMAL);
	auto up_plane_index = geometryDataHandles.size() - 1;
	// down
	geometryDataHandles.push_back(utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::forward * 90.0f));
	utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::down * 0.5f);
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::back * 90.0f), core::data::geometry::constants::NORMAL);
	auto down_plane_index = geometryDataHandles.size() - 1;
	// left
	geometryDataHandles.push_back(utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::down * 90.0f));
	utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::left * 0.5f);
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::up * 90.0f), core::data::geometry::constants::NORMAL);
	auto left_plane_index = geometryDataHandles.size() - 1;
	// right
	geometryDataHandles.push_back(utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::up * 90.0f));
	utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::right * 0.5f);
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::down * 90.0f), core::data::geometry::constants::NORMAL);
	auto right_plane_index = geometryDataHandles.size() - 1;
	// forward
	geometryDataHandles.push_back(utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::forward * 0.5f);
	auto forward_plane_index = geometryDataHandles.size() - 1;
	// back
	geometryDataHandles.push_back(utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::left * 90.0f));
	utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::back * 0.5f);
	utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
							  psl::math::from_euler(psl::vec3::right * 90.0f), core::data::geometry::constants::NORMAL);
	auto back_plane_index = geometryDataHandles.size() - 1;
	for(auto& handle : geometryDataHandles)
	{
		core::stream colorstream{core::stream::type::vec3};
		auto& colors = colorstream.as_vec3().value().get();
		auto& normalStream =
			handle->vertices(core::data::geometry::constants::NORMAL).value().get().as_vec3().value().get();
		colors.resize(normalStream.size());
		std::memcpy(colors.data(), normalStream.data(), sizeof(psl::vec3) * normalStream.size());

		std::for_each(std::begin(colors), std::end(colors), [](auto& color) {
			color =
				(psl::math::dot(psl::math::normalize(color), psl::math::normalize(psl::vec3(145, 170, 35))) + 1.0f) *
				0.5f;
			// color = std::max((color[0] + color[1] + color[2]), 0.0f) + 0.33f;
		});
		handle->vertices(core::data::geometry::constants::COLOR, colorstream);
		handle->erase(core::data::geometry::constants::TANGENT);
		handle->erase(core::data::geometry::constants::NORMAL);
		geometryHandles.emplace_back(cache.create<gfx::geometry>(context_handle, handle, vertexBuffer, indexBuffer));
	}

	// create the buffer that we'll use for storing the WVP for the shaders;
	auto frameCamBufferData =
		cache.create<data::buffer>(core::gfx::memory_usage::uniform_buffer,
								   core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
								   resource_region
									   .create_region(sizeof(core::ecs::systems::gpu_camera::framedata) * 128,
													  uniform_buffer_align, new memory::default_allocator(true))
									   .value());

	auto frameCamBuffer = cache.create<gfx::buffer>(context_handle, frameCamBufferData);
	cache.library().set(frameCamBuffer, "GLOBAL_WORLD_VIEW_PROJECTION_MATRIX");


	// create a pipeline cache
	auto pipeline_cache = cache.create<core::gfx::pipeline_cache>(context_handle);

	psl::array<core::resource::handle<core::gfx::material>> materials;
	core::resource::handle<core::gfx::material> depth_material =
		setup_gfx_depth_material(cache, context_handle, pipeline_cache, matBuffer);

	// water
	materials.emplace_back(
		setup_gfx_material(cache, context_handle, pipeline_cache, matBuffer, "3982b466-58fe-4918-8735-fc6cc45378b0"_uid,
						   "4429d63a-9867-468f-a03f-cf56fee3c82e"_uid, "e14da8e3-1e03-a635-7889-a1b0f27f36bb"_uid));

	// grass
	materials.emplace_back(
		setup_gfx_material(cache, context_handle, pipeline_cache, matBuffer, "3982b466-58fe-4918-8735-fc6cc45378b0"_uid,
						   "4429d63a-9867-468f-a03f-cf56fee3c82e"_uid, "3b47e2f3-1faa-f668-65d6-4aa32d04dab4"_uid));

	// dirt
	materials.emplace_back(
		setup_gfx_material(cache, context_handle, pipeline_cache, matBuffer, "3982b466-58fe-4918-8735-fc6cc45378b0"_uid,
						   "4429d63a-9867-468f-a03f-cf56fee3c82e"_uid, "fb47fd91-8bd8-ba29-928f-9666404a399f"_uid));

	// rock
	materials.emplace_back(
		setup_gfx_material(cache, context_handle, pipeline_cache, matBuffer, "3982b466-58fe-4918-8735-fc6cc45378b0"_uid,
						   "4429d63a-9867-468f-a03f-cf56fee3c82e"_uid, "e848362f-fb4a-408f-2598-3378365d8da1"_uid));

	psl::array<core::resource::handle<core::gfx::bundle>> bundles;
	bundles.emplace_back(cache.create<gfx::bundle>(instanceBuffer));
	bundles[bundles.size() - 1]->set(materials[0], 2000);

	bundles.emplace_back(cache.create<gfx::bundle>(instanceBuffer));
	bundles[bundles.size() - 1]->set(materials[1], 2000);
	bundles[bundles.size() - 1]->set(depth_material, 1000);

	bundles.emplace_back(cache.create<gfx::bundle>(instanceBuffer));
	bundles[bundles.size() - 1]->set(materials[2], 2000);

	bundles.emplace_back(cache.create<gfx::bundle>(instanceBuffer));
	bundles[bundles.size() - 1]->set(materials[3], 2000);

	core::gfx::render_graph renderGraph{};
	auto swapchain_pass = renderGraph.create_drawpass(context_handle, swapchain_handle);

	{
		psl::UID compute_texture_uid;
		std::unique_ptr<core::meta::texture> texture = std::make_unique<core::meta::texture>();
		texture->width(512);
		texture->height(512);
		texture->format(core::gfx::format::r8g8b8a8_unorm);
		auto handle			= cache.create_using<core::gfx::texture>(std::move(texture), context_handle);
		compute_texture_uid = handle.meta()->ID();

		auto compute_handle = create_compute(cache, context_handle, pipeline_cache,
											 "594b2b8a-d4ea-e162-2b2c-987de571c7be"_uid, compute_texture_uid);

		if(context_handle->backend() == core::gfx::graphics_backend::gles)
		{
			auto compute_pass = renderGraph.create_computepass(context_handle);
			compute_pass->add(core::gfx::computecall{compute_handle});
			renderGraph.connect(compute_pass, swapchain_pass);
		}
	}
	// create the ecs
	using psl::ecs::state;

	state ECSState{};

	using namespace core::ecs::components;

	const size_t area			  = 128;
	const size_t area_granularity = 128;
	const size_t size_steps		  = 24;
	const float timeScale = 1.f;
	const size_t spawnInterval = (size_t)((float)100 * (1.f/timeScale));

	utility::platform::file::write(utility::application::path::get_path() + "frame_data.txt",
								   core::profiler.to_string());


	core::ecs::components::transform camTrans{psl::vec3{40, 15, 150}};
	camTrans.rotation = psl::math::look_at_q(camTrans.position, psl::vec3::zero, psl::vec3::up);


	core::ecs::systems::render render_system{ECSState, swapchain_pass};
	render_system.add_render_range(2000, 3000);
	core::ecs::systems::fly fly_system{ECSState, surface_handle->input()};
	core::ecs::systems::gpu_camera gpu_camera_system{ECSState, surface_handle, frameCamBuffer,
													 context_handle->backend()};

	ECSState.declare(psl::ecs::threading::par, core::ecs::systems::movement);
	ECSState.declare(psl::ecs::threading::par, core::ecs::systems::lifetime);
	/*ECSState.declare(psl::ecs::threading::par, [](info& info, pack<transform, const lifetime> pack) {
		for(auto [transf, life] : pack)
		{
			auto remainder = std::min(life.value * 2.0f, 1.0f);
			transf.scale *= remainder;
		}
	});*/

	ECSState.declare(psl::ecs::threading::par, core::ecs::systems::attractor);
	core::ecs::systems::geometry_instancing geometry_instancing_system{ECSState};

	/*core::ecs::systems::lighting_system lighting{
		psl::view_ptr(&ECSState), psl::view_ptr(&cache), resource_region, psl::view_ptr(&renderGraph),
		swapchain_pass,			  context_handle,		 surface_handle};*/

	core::ecs::systems::text text{ECSState,	   cache,		   context_handle, vertexBuffer,
								  indexBuffer, pipeline_cache, matBuffer,	   instanceBuffer};

	auto eCam = ECSState.create(1, std::move(camTrans), psl::ecs::empty<core::ecs::components::camera>{},
								psl::ecs::empty<core::ecs::components::input_tag>{});

	size_t iterations										 = 25600;
	std::chrono::high_resolution_clock::time_point last_tick = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point next_spawn = std::chrono::high_resolution_clock::now();
	/*ECSState.create(
		1,
		[&bundles, &geometryHandles](core::ecs::components::renderable& renderable) {
			renderable = {(std::rand() % 2 == 0) ? bundles[0] : bundles[1],
						  geometryHandles[std::rand() % geometryHandles.size()]};
		},
		psl::ecs::empty<core::ecs::components::transform>{});*/


	ECSState.create(1, psl::ecs::empty<core::ecs::components::transform>{},
					core::ecs::components::light{{}, 1.0f, core::ecs::components::light::type::DIRECTIONAL, true});

	std::array<int, 4> matusage{0};

	std::string str{"dsff"};
	auto textEntity = ECSState.create(1, core::ecs::components::text{str.data()},
									  psl::ecs::empty<core::ecs::components::dynamic_tag>());

	size_t frame{0};
	std::chrono::duration<float> elapsed{};

#ifdef _DEBUG
	size_t count = 5;
	size_t swing = 0;
#else
	size_t count = 750;
	size_t swing = 100;
#endif
	while(surface_handle->tick())
	{
		core::log->info("---- FRAME {0} START ----", frame);
		core::log->info("There are {} renderables alive right now", ECSState.count<renderable>());
		core::profiler.next_frame();

		core::profiler.scope_begin("system tick");
		ECSState.tick(elapsed * timeScale);
		core::profiler.scope_end();

		core::profiler.scope_begin("presenting");
		renderGraph.present();
		core::profiler.scope_end();

		core::profiler.scope_begin("creating entities");


		str = "material 0: " + std::to_string(matusage[0]) + " material 1: " + std::to_string(matusage[1]) +
			  " material 2: " + std::to_string(matusage[2]) + " material 3: " + std::to_string(matusage[3]);
		ECSState.set_component(textEntity, core::ecs::components::text{str.data()});

		core::profiler.scope_end();

		auto current_time = std::chrono::high_resolution_clock::now();
		elapsed			  = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_tick);

		//while (current_time >= next_spawn)
		{
			next_spawn += std::chrono::milliseconds(spawnInterval);
			ECSState.create(
				(iterations > 0) ? count + std::rand() % (swing + 1) : 0,
				[&bundles, &geometryHandles, &matusage](core::ecs::components::renderable& renderable) {
					auto matIndex = (std::rand() % 2 == 0);
					matusage[matIndex] += 1;
					renderable = { bundles[matIndex], geometryHandles[/*std::rand() % geometryHandles.size()*/0] };
				},
				psl::ecs::empty<core::ecs::components::dynamic_tag>{}, psl::ecs::empty<core::ecs::components::transform>{},
					[](core::ecs::components::lifetime& target) { target = { 0.5f + ((std::rand() % 50) / 50.0f) * 2.0f}; },
					[&size_steps](core::ecs::components::velocity& target) {
					target = { psl::math::normalize(psl::vec3((float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
															 (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
															 (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f)),
							  ((std::rand() % 5000) / 500.0f) * 8.0f, 1.0f };
				});


			if (iterations > 0)
			{
				if (ECSState.filter<core::ecs::components::attractor>().size() < 1)
				{
					ECSState.create(
						2,
						[](core::ecs::components::lifetime& target) {
							target = { 5.0f + ((std::rand() % 50) / 50.0f) * 5.0f };
						},
						[&size_steps](core::ecs::components::attractor& target) {
							target = { (float)(std::rand() % size_steps) / size_steps * 3 + 0.5f,
									  (float)(std::rand() % size_steps) / size_steps * 80 };
						},
							[&area_granularity, &area, &size_steps](core::ecs::components::transform& target) {
							target = {
								psl::vec3(
									(float)((float)(std::rand() % (area * area_granularity)) / (float)area_granularity) -
										(area / 2.0f),
									(float)((float)(std::rand() % (area * area_granularity)) / (float)area_granularity) -
										(area / 2.0f),
									(float)((float)(std::rand() % (area * area_granularity)) / (float)area_granularity) -
										(area / 2.0f)),

								psl::vec3((float)(std::rand() % size_steps) / size_steps,
										  (float)(std::rand() % size_steps) / size_steps,
										  (float)(std::rand() % size_steps) / size_steps) };
						});
				}
				--iterations;
			}
		}
		last_tick = current_time;
		core::log->info("---- FRAME {0} END   ---- duration {1} ms", frame++, elapsed.count() * 1000);
	}

	return 0;
}

int main()
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	setup_loggers();

#ifdef _MSC_VER
	{ // here to trick the compiler into generating these types to get UUID natvis support
		dummy::hex_dummy_high hex_dummy_high{};
		dummy::hex_dummy_low hex_dummy_lowy{};
	}
#endif

	psl::vec4 v{0.f, 1.f, 2.f, 3.f};
	// auto res{ v.xy + v.zy + v.zz };
	// std::thread vk_thread(entry, graphics_backend::gles);
	// std::thread gl_thread(entry, graphics_backend::vulkan);
	std::srand(0);
	return entry(graphics_backend::gles);
	// gl_thread.join();
	// return 0;
	std::srand(0);
	return entry(graphics_backend::vulkan);
}
#endif

#endif