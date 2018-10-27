// core.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "header_info.h"
#ifdef CORE_EXECUTABLE
#include "data/window.h" // application data
#include "os/surface.h"  // the OS surface to draw one
#include "vk/context.h"  // the vulkan context
//#include "systems\resource.h" // resource system
#include "vk/swapchain.h" // the gfx swapchain which we'll use as our backbuffer
#include "gfx/pass.h"

#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

// meta test shader
#include "meta/shader.h"

#include "data/material.h"
#include "gfx/material.h"
#include "data/geometry.h"

// drawgroup
#include "gfx/drawgroup.h"


using namespace core;
using namespace core::resource;
using namespace core::gfx;
using namespace core::os;

//#define DEDICATED_GRAPHICS_THREAD

void setup_loggers()
{
	if(!utility::platform::file::exists(utility::application::path::get_path() + "logs/main.log"))
		utility::platform::file::write(utility::application::path::get_path() + "logs/main.log", "");
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + "logs/main.log", true));
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	auto logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));


	spdlog::register_logger(logger);
	core::log		   = logger;
	core::ivk::log	 = logger;
	core::gfx::log	 = logger;
	core::systems::log = logger;
	core::os::log	  = logger;
	core::data::log	= logger;
}

#if defined(MULTI_CONTEXT_RENDERING)
#include <atomic>
#include <shared_mutex>
namespace core::systems
{
	class renderer_view;

	class renderer
	{
		uint32_t lock(uint32_t v)
		{
			mutex.lock();
			return v;
		}

	  public:
		renderer(cache* cache) : m_Cache(cache), deviceIndex(lock(0)), m_Thread(&core::systems::renderer::main, this) {}
		~renderer();
		renderer(renderer&& other) = delete;
		renderer& operator=(renderer&& other) = delete;
		renderer(const renderer& other)		  = delete;
		renderer& operator=(const renderer& other) = delete;

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
		psl::string8_tstream ss;
		ss << m_Thread.get_id();
		// Utility::OS::RegisterThisThread("RenderThread " + ss.str());
		m_Context = create<context>(*m_Cache);
		if(!m_Context.load(APPLICATION_FULL_NAME, deviceIndex))
		{
			LOG_FATAL << "Could not create graphics API surface to use for drawing.";
			return throw std::runtime_error("no vulkan context could be created");
		}
		mutex.unlock();
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
	psl::string libraryPath{utility::application::path::library + _T("resources.metalib"_sv)};

	cache* cache = new core::resource::cache(meta::library{psl::from_string8_t(libraryPath)}, 1024u * 1024u * 20u, 4u,
											 new memory::default_allocator());

	auto window_data = create_shared<data::window>(*cache, UID::convert("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"));
	window_data.load();

	auto rend = new core::systems::renderer{cache};
	for(auto i = 0u; i < views; ++i)
	{
		auto window_handle = create<surface>(*cache);
		if(!window_handle.load(window_data))
		{
			LOG_FATAL << "Could not create a OS surface to draw on.";
			throw std::runtime_error("no OS surface could be created");
		}
		rend->create_view(window_handle);
	}
	return rend;
}

int main()
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	Utility::Logger::Init("Main");
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

	cache cache{::meta::library{psl::from_string8_t(libraryPath)}, 1024u * 1024u * 20u, 4u,
				new memory::default_allocator()};

	auto window_data = create<data::window>(cache, UID::convert("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"));
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


int main()
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	setup_loggers();

	core::stream stream{core::stream::type::vec3};
	if(auto res = stream.as_vec3(); res)
	{
		std::vector<glm::vec3>& stream_res = res.value();
		stream_res.push_back(glm::vec3{0, 1, 0});
		stream_res.push_back(glm::vec3{2, 0, 3});
	}


	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	cache cache{::meta::library{psl::to_string8_t(libraryPath)}, 1024u * 1024u * 20u, 4u,
				new memory::default_allocator()};


	core::data::geometry geom{UID::generate(), cache};
	serialization::serializer s;
	format::container container;
	// s.serialize<serialization::encode_to_format>(stream, container);
	s.serialize<serialization::encode_to_format>(geom, container);
	auto res = container.to_string();

	auto mat = core::resource::create<core::gfx::material>(cache);
	// mat.load(core::resource::create<core::data::material>(cache, mat.ID()));

	cache.find<core::data::material>(mat.RUID());

	auto window_data = create<data::window>(cache, UID::convert("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"));
	window_data.load();

	auto window_handle = create<surface>(cache);
	if(!window_handle.load(window_data))
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
	swapchain_handle.load(window_handle, context_handle);
	window_handle->register_swapchain(swapchain_handle);
	context_handle->device().waitIdle();
	pass pass{context_handle, swapchain_handle};

	core::gfx::drawgroup dGroup{};

	auto& default_layer = dGroup.layer("default", 0);
	auto& call = dGroup.add(default_layer, mat);



	uint64_t frameCount = 0u;
	while(window_handle->tick())
	{
		if(window_handle->open() && swapchain_handle->is_ready())
		{
			pass.prepare();
			pass.present();
		}

		++frameCount;
	}

	return 0;
}
#endif
#endif