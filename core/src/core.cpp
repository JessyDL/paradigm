// core.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#ifdef CORE_EXECUTABLE
#include "header_info.h"
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

// drawgroup
#include "gfx/drawgroup.h"

// hello triangle
#include "data/buffer.h"
#include "data/geometry.h"
#include "data/material.h"
#include "vk/buffer.h"
#include "vk/geometry.h"
#include "gfx/material.h"
#include "gfx/pipeline_cache.h"
#include "meta/shader.h"

// hello texture
#include "meta/texture.h"
#include "vk/texture.h"
#include "data/sampler.h"
#include "vk/sampler.h"

// glm rotate
#include "glm/gtx/rotate_vector.hpp"

#include "systems/input.h"

#include "utility/geometry.h"

#include "ecs/ecs.hpp"


using namespace core;
using namespace core::resource;
using namespace core::gfx;
using namespace core::os;

struct framedata
{
	glm::mat4 clipMatrix;
	glm::mat4 projectionMatrix;
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 WVP;
	glm::mat4 VP;
	glm::vec4 ScreenParams;
	glm::vec4 GameTimer;
	glm::vec4 Parameters;
	glm::vec4 FogColor;
	glm::vec4 viewPos;
	glm::vec4 viewDir;
};

class transform
{
  public:
	const glm::vec3& position() const noexcept { return m_Position; };
	const glm::quat& rotation() const noexcept { return m_Rotation; };
	const glm::vec3& scale() const noexcept { return m_Scale; };
	const glm::vec3& up() const noexcept { return m_Up; };
	const glm::vec3& direction() const noexcept { return m_Direction; };

	void position(const glm::vec3& value) { m_Position = value; };
	void rotation(const glm::quat& value)
	{
		m_Direction = glm::normalize(glm::rotate(value, m_Direction));
		m_Rotation  = value;
	};
	void scale(const glm::vec3& value) { m_Scale = value; };
	void up(const glm::vec3& value) { m_Up = glm::normalize(value); };
	void direction(const glm::vec3& value)
	{
		m_Direction = glm::normalize(value);
		m_Rotation  = glm::quat(glm::radians(m_Direction));
	};

	void rotate(const glm::detail::float32& degrees, const glm::vec3& axis)
	{
		m_Rotation  = m_Rotation * glm::normalize(glm::angleAxis(degrees, axis));
		m_Direction = glm::rotate(m_Direction, degrees, axis);
	}

  private:
	glm::vec3 m_Position{glm::vec3::Zero};
	glm::vec3 m_Scale{glm::vec3::One};
	glm::quat m_Rotation{1, 0, 0, 0};
	glm::vec3 m_Up{glm::vec3::Up};
	glm::vec3 m_Direction{glm::vec3::Forward};
};

class camera
{
	const glm::mat4 clip{1.0f,  0.0f, 0.0f, 0.0f, +0.0f, -1.0f, 0.0f, 0.0f,
						 +0.0f, 0.0f, 0.5f, 0.0f, +0.0f, 0.0f,  0.5f, 1.0f};

  public:
	camera(core::resource::handle<core::gfx::buffer> buffer, core::resource::handle<core::os::surface> surface)
		: fdatasegment(buffer->reserve(sizeof(framedata)).value()), m_Buffer(buffer), m_Surface(surface)
	{
		m_Transform.position(glm::vec3(0, 0, -5.0f));
		update_buffer();
	}

	void tick(std::chrono::duration<float> dTime) { update_buffer(); }

	transform& transform() { return m_Transform; }

	const std::vector<core::gfx::drawlayer>& layers() const noexcept { return m_Layers; }
	void layers(const std::vector<core::gfx::drawlayer>& layers) noexcept { m_Layers = layers; }
  private:
	void update_buffer()
	{
		fdata.ScreenParams =
			glm::vec4((float)m_Surface->data().width(), (float)m_Surface->data().height(), m_Near, m_Far);

		fdata.projectionMatrix = glm::perspective(
			glm::radians(m_FOV), (float)m_Surface->data().width() / (float)m_Surface->data().height(), m_Near, m_Far);
		fdata.clipMatrix = clip;

		fdata.viewMatrix =
			glm::lookAt(m_Transform.position(), m_Transform.position() + m_Transform.direction(), m_Transform.up());
		fdata.modelMatrix = glm::mat4();
		fdata.viewPos	 = glm::vec4(m_Transform.position(), 1.0);
		fdata.viewDir	 = glm::vec4(m_Transform.direction(), 0.0);

		fdata.VP  = fdata.clipMatrix * fdata.projectionMatrix * fdata.viewMatrix;
		fdata.WVP = fdata.VP * fdata.modelMatrix;
		m_Buffer->commit({{&fdata, fdatasegment}});
	}
	memory::segment fdatasegment;
	core::resource::handle<core::gfx::buffer> m_Buffer;
	core::resource::handle<core::os::surface> m_Surface;
	framedata fdata{};

	::transform m_Transform{};
	float m_FOV{60};
	float m_Near = 0.1f;
	float m_Far  = 24.f;
	std::vector<core::gfx::drawlayer> m_Layers;
};

class transform_inputlistener
{
  public:
	transform_inputlistener(transform& transform, core::systems::input& inputSystem)
		: m_Transform(transform), m_InputSystem(inputSystem)
	{
		m_InputSystem.subscribe(this);
	}

	~transform_inputlistener() { m_InputSystem.unsubscribe(this); }
	void on_key_pressed(core::systems::input::keycode keyCode)
	{
		using keycode_t = core::systems::input::keycode;
		switch(keyCode)
		{
		case keycode_t::Z: { m_Moving[0] = true;
		}
		break;
		case keycode_t::Q: { m_Moving[2] = true;
		}
		break;
		case keycode_t::S: { m_Moving[1] = true;
		}
		break;
		case keycode_t::D: { m_Moving[3] = true;
		}
		break;
		case keycode_t::SPACE: { m_Up = true;
		}
		break;
		default: break;
		}
	}

	void on_key_released(core::systems::input::keycode keyCode)
	{
		using keycode_t = core::systems::input::keycode;
		switch(keyCode)
		{
		case keycode_t::Z: { m_Moving[0] = false;
		}
		break;
		case keycode_t::Q: { m_Moving[2] = false;
		}
		break;
		case keycode_t::S: { m_Moving[1] = false;
		}
		break;
		case keycode_t::D: { m_Moving[3] = false;
		}
		break;
		case keycode_t::SPACE: { m_Up = false;
		}
		break;
		default: break;
		}
	}

	void on_mouse_move(core::systems::input::mouse_delta delta)
	{
		if(!m_AllowRotating) return;
		m_MouseTargetX += delta.x;
		m_MouseTargetY += delta.y;
	}

	void on_scroll(core::systems::input::scroll_delta delta)
	{
		m_MoveSpeed += m_MoveStepIncrease * delta.y;
		m_MoveSpeed = std::max(m_MoveSpeed, 0.05f);
	}

	void on_mouse_pressed(core::systems::input::mousecode mCode)
	{
		if(mCode == core::systems::input::mousecode::RIGHT) m_AllowRotating = true;
	}

	void on_mouse_released(core::systems::input::mousecode mCode)
	{
		if(mCode == core::systems::input::mousecode::RIGHT) m_AllowRotating = false;
	}

	void tick(std::chrono::duration<float> dTime)
	{
		// m_Transform.position(m_Transform.position() + (m_MoveVector * dTime.count()));
		if(m_Moving[0])
		{
			glm::vec3 forward = m_Transform.direction();
			// forward.y = 0;
			forward = glm::normalize(forward);
			m_Transform.position(m_Transform.position() + forward * m_MoveSpeed * dTime.count());
		}

		if(m_Moving[1])
		{
			glm::vec3 forward = m_Transform.direction();
			// forward.y = 0;
			forward = glm::normalize(forward);
			m_Transform.position(m_Transform.position() - forward * m_MoveSpeed * dTime.count());
		}

		if(m_Moving[2])
		{
			glm::vec3 Left = glm::cross(m_Transform.direction(), m_Transform.up());
			Left		   = glm::normalize(Left);
			Left *= m_MoveSpeed * dTime.count();
			m_Transform.position(m_Transform.position() - Left);
		}

		if(m_Moving[3])
		{
			glm::vec3 Right = glm::cross(m_Transform.up(), m_Transform.direction());
			Right			= glm::normalize(Right);
			Right *= m_MoveSpeed * dTime.count();
			m_Transform.position(m_Transform.position() - Right);
		}

		if(m_Up)
		{
			m_Transform.position(m_Transform.position() + glm::vec3::Up * dTime.count() * 1.0f * m_MoveSpeed);
		}

		if(m_MouseX != m_MouseTargetX || m_MouseY != m_MouseTargetY)
		{
			float mouseSpeed = 0.004f;
			auto diffX		 = m_MouseX - m_MouseTargetX;
			auto diffY		 = m_MouseY - m_MouseTargetY;
			m_AngleH		 = mouseSpeed * diffX;
			ChangeHeading(-m_AngleH);
			core::log->info(m_AngleH);

			m_AngleV = mouseSpeed * diffY;
			ChangePitch(m_AngleV);
			m_MouseX = m_MouseTargetX;
			m_MouseY = m_MouseTargetY;

			// detmine axis for pitch rotation
			glm::vec3 axis = glm::cross(m_Transform.direction(), m_Transform.up());
			// compute quaternion for pitch based on the camera pitch angle
			glm::quat pitch_quat = glm::angleAxis(m_Pitch, axis);
			// determine heading quaternion from the camera up vector and the heading angle
			glm::quat heading_quat = glm::angleAxis(m_Heading, m_Transform.up());
			// add the two quaternions
			glm::quat temp = glm::cross(pitch_quat, heading_quat);
			m_Transform.rotation(glm::normalize(temp));

			m_AngleH  = 0;
			m_AngleV  = 0;
			m_Pitch   = 0;
			m_Heading = 0;
		}
	}

  private:
	void ChangePitch(float degrees)
	{
		m_Pitch += degrees;

		// Check bounds for the camera pitch
		if(m_Pitch > 360.0f)
		{
			m_Pitch -= 360.0f;
		}
		else if(m_Pitch < -360.0f)
		{
			m_Pitch += 360.0f;
		}
	}

	void ChangeHeading(float degrees)
	{ // This controls how the heading is changed if the camera is pointed straight up or down
		// The heading delta direction changes
		if((m_Pitch > 90 && m_Pitch < 270) || (m_Pitch < -90 && m_Pitch > -270))
		{
			m_Heading += degrees;
		}
		else
		{
			m_Heading -= degrees;
		}
		// Check bounds for the camera heading
		if(m_Heading > 360.0f)
		{
			m_Heading -= 360.0f;
		}
		else if(m_Heading < -360.0f)
		{
			m_Heading += 360.0f;
		}
	}


	std::array<bool, 4> m_Moving{false};
	bool m_Up{false};
	glm::vec3 m_MoveVector{};
	transform& m_Transform;
	core::systems::input& m_InputSystem;

	int64_t m_MouseX{1};
	int64_t m_MouseY{1};

	int64_t m_MouseTargetX{1};
	int64_t m_MouseTargetY{1};

	float m_Pitch;
	float m_Heading;
	float m_AngleH;
	float m_AngleV;

	float m_MoveSpeed{1.0f};
	const float m_MoveStepIncrease{0.035f};
	bool m_AllowRotating{false};
};

class renderer
{
  public:
	renderer(resource::cache& cache, const UID& geometry, const UID& material)
	{
		m_Geometry = cache.find<core::gfx::geometry>(geometry);
		m_Material = cache.find<core::gfx::material>(material);
	};
	renderer(resource::handle<core::gfx::geometry>& geometry, resource::handle<core::gfx::material>& material)
		: m_Geometry(geometry), m_Material(material){};

	transform& transform() noexcept { return m_Transform; }

	void tick(std::chrono::duration<float> dTime) { }
  private:
	::transform m_Transform;
	resource::handle<core::gfx::geometry> m_Geometry;
	resource::handle<core::gfx::material> m_Material;
};

class world
{
  public:
	world() = default;

	template <typename T, typename... Args>
	T& add(Args&&... args)
	{
		if constexpr (std::is_same<T, camera>::value)
		{
			auto& dGroup = m_Drawgroups.emplace_back();
			auto& layer = dGroup.layer("default", 0);
			auto& c = m_Cameras.emplace_back(std::forward<Args>(args)...);
			c.layers({ layer });
			return c;
		}
		else if constexpr (std::is_same<T, renderer>::value)
		{
			return m_Renderers.emplace_back(std::forward<Args>(args)...);
		}
		else
		{
			static_assert(utility::templates::always_false{}, "please select a type that is supported");
		}
	}

	void tick(std::chrono::duration<float> dTime) 
	{ 
		for (auto& c : m_Cameras)
		{
			c.tick(dTime);
		}
		for (auto& r : m_Renderers)
		{
			r.tick(dTime);
		}
	}

	void set_visible_to(const renderer& renderer, const camera& camera)
	{
		for (const auto& c : m_Cameras)
		{
			//if(c.)
		}
	}

  private:
	std::vector<renderer> m_Renderers;
	std::vector<camera> m_Cameras;
	std::vector<core::gfx::drawgroup> m_Drawgroups;
};

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
	auto mainlogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "main.log", true);
	auto outlogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();


	auto ivklogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "ivk.log", true);

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
	sinks.push_back(outlogger);
	sinks.push_back(corelogger);

	auto logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
	spdlog::register_logger(logger);
	core::log = logger;


	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(outlogger);
	sinks.push_back(systemslogger);

	auto system_logger = std::make_shared<spdlog::logger>("systems", begin(sinks), end(sinks));
	spdlog::register_logger(system_logger);
	core::systems::log = system_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(outlogger);
	sinks.push_back(oslogger);

	auto os_logger = std::make_shared<spdlog::logger>("os", begin(sinks), end(sinks));
	spdlog::register_logger(os_logger);
	core::os::log = os_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(outlogger);
	sinks.push_back(datalogger);

	auto data_logger = std::make_shared<spdlog::logger>("data", begin(sinks), end(sinks));
	spdlog::register_logger(data_logger);
	core::data::log = data_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(outlogger);
	sinks.push_back(gfxlogger);

	auto gfx_logger = std::make_shared<spdlog::logger>("gfx", begin(sinks), end(sinks));
	spdlog::register_logger(gfx_logger);
	core::gfx::log = gfx_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(outlogger);
	sinks.push_back(ivklogger);

	auto ivk_logger = std::make_shared<spdlog::logger>("ivk", begin(sinks), end(sinks));
	spdlog::register_logger(ivk_logger);
	core::ivk::log = ivk_logger;
}
#else
#include "spdlog/sinks/android_sink.h"
void setup_loggers()
{
	core::log		   = spdlog::android_logger_mt("main");
	core::systems::log = spdlog::android_logger_mt("systems");
	core::os::log	  = spdlog::android_logger_mt("os");
	core::data::log	= spdlog::android_logger_mt("data");
	core::gfx::log	 = spdlog::android_logger_mt("gfx");
	core::ivk::log	 = spdlog::android_logger_mt("ivk");
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


#if defined(PLATFORM_ANDROID)
bool focused{true};
int android_entry()
{
	setup_loggers();
	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	cache cache{::meta::library{psl::to_string8_t(libraryPath)}, resource_region.allocator()};

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

struct float_system
{
	core::ecs::range<float> m_Floats;

	void announce(core::ecs::state& state)
	{
		state.register_rw_range(m_Floats);
	}
	
	void tick(std::chrono::duration<float> dTime)
	{

	}
};
int entry()
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	setup_loggers();


	std::vector<float> fl_v{ {5.0f, 9.3f, 12.6f, 44.f, 4211689.0f, 78.542f, 99.f} };
	std::vector<size_t> fl_i{ {0,5,2,3,6} };
	core::ecs::range<float> fl_range{ fl_v, fl_i };

	for (auto i : fl_range)
	{
		i += 5.0f;
		core::log->info("{}", i);
	}

	core::ecs::state state;
	auto e{state.create()};
	auto e2{state.create()};
	auto e3{state.create()};
	auto e_list{ state.create(100000) };
	state.add_component<float>(e, 2.0f);
	state.add_component<float>(e);
	state.add_component<float>(e_list);
	state.add_component<float>(e_list);
	state.remove_component<float>(e);
	state.add_component<float>(e);
	state.add_component<int>(e,5);
	auto& int_comp = state.get_component<int>(e);
	int_comp += 10;
	state.add_component<int>(e3,3);
	state.add_component<int>(e,1);
	state.add_component<float>(e,5.0f);
	state.add_component<float>(e2);
	state.add_component<float>(e3);
	state.add_component<uint8_t>(e, uint8_t{ 0u });
	state.add_component<uint8_t>(e2, uint8_t{ 1u });

	typedef uint8_t name_t;
	auto& uint8_t_comp = state.get_component<name_t>(e2);
	uint8_t_comp += 99;

	auto res = state.filter<float, int>();

	std::vector<float> filter_range_fl;
	std::vector<uint8_t> filter_range_uint;
	res = state.filter<float, uint8_t>(filter_range_fl, filter_range_uint);

	float_system fl_system{};
	state.register_system(fl_system);
	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	cache cache{::meta::library{psl::to_string8_t(libraryPath)}, resource_region.allocator()};

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

	// create a staging buffer, this is allows for more advantagous resource access for the GPU
	auto stagingBufferData = create<data::buffer>(cache);
	stagingBufferData.load(vk::BufferUsageFlagBits::eTransferSrc,
						   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
						   memory::region{1024 * 1024 * 32, 4, new memory::default_allocator(false)});
	auto stagingBuffer = create<gfx::buffer>(cache);
	stagingBuffer.load(context_handle, stagingBufferData);

	// create the buffer that we'll use for storing the WVP for the shaders;
	auto frameBufferData = create<data::buffer>(cache);
	frameBufferData.load(vk::BufferUsageFlagBits::eUniformBuffer,
						 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
						 resource_region
							 .create_region(sizeof(framedata) * 128,
											context_handle->properties().limits.minUniformBufferOffsetAlignment,
											new memory::default_allocator(true))
							 .value());
	// memory::region{sizeof(framedata)*128, context_handle->properties().limits.minUniformBufferOffsetAlignment, new
	// memory::default_allocator(true)});
	auto frameBuffer = create<gfx::buffer>(cache);
	frameBuffer.load(context_handle, frameBufferData);
	cache.library().set(frameBuffer.ID(), "GLOBAL_WORLD_VIEW_PROJECTION_MATRIX");

	// create the buffers to store the model in
	// - memory region which we'll use to track the allocations, this is supposed to be virtual as we don't care to have
	// a copy on the CPU
	// - then we create the vulkan buffer resource to interface with the GPU
	auto geomBufferData = create<data::buffer>(cache);
	geomBufferData.load(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer |
							vk::BufferUsageFlagBits::eTransferDst,
						vk::MemoryPropertyFlagBits::eDeviceLocal,
						memory::region{1024 * 1024, 4, new memory::default_allocator(false)});
	auto geomBuffer = create<gfx::buffer>(cache);
	geomBuffer.load(context_handle, geomBufferData, stagingBuffer);

	// load the example model
	// auto geomData = create<data::geometry>(cache, UID::convert("bf36d6f1-af53-41b9-b7ae-0f0cb16d8734"));
	auto geomData = utility::geometry::create_box(cache, glm::vec3::One);
	geomData.load();
	auto& positionstream =
		geomData->vertices(core::data::geometry::constants::POSITION).value().get().as_vec3().value().get();
	core::stream colorstream{core::stream::type::vec3};
	auto& colors			  = colorstream.as_vec3().value().get();
	const float range		  = 1.0f;
	const bool inverse_colors = false;
	for(auto i = 0; i < positionstream.size(); ++i)
	{
		if(inverse_colors)
		{
			float red   = std::max(-range, std::min(range, positionstream[i].x)) / range;
			float green = std::max(-range, std::min(range, positionstream[i].y)) / range;
			float blue  = std::max(-range, std::min(range, positionstream[i].z)) / range;
			colors.emplace_back(glm::vec3(red, green, blue));
		}
		else
		{
			float red   = (std::max(-range, std::min(range, positionstream[i].x)) + range) / (range * 2);
			float green = (std::max(-range, std::min(range, positionstream[i].y)) + range) / (range * 2);
			float blue  = (std::max(-range, std::min(range, positionstream[i].z)) + range) / (range * 2);
			colors.emplace_back(glm::vec3(red, green, blue));
		}
	}

	geomData->vertices(core::data::geometry::constants::COLOR, colorstream);
	auto geometry = create<gfx::geometry>(cache);
	geometry.load(context_handle, geomData, geomBuffer, geomBuffer);

	// get a vertex and fragment shader that can be combined, we only need the meta
	if(!cache.library().contains(UID::convert("f889c133-1ec0-44ea-9209-251cd236f887")) ||
	   !cache.library().contains(UID::convert("4429d63a-9867-468f-a03f-cf56fee3c82e")))
	{
		core::log->critical(
			"Could not find the required shader resources in the meta library. Did you forget to copy the files over?");
		if(surface_handle) surface_handle->terminate();
		return -1;
	}
	auto vertShaderMeta =
		cache.library().get<core::meta::shader>(UID::convert("f889c133-1ec0-44ea-9209-251cd236f887")).value();
	auto fragShaderMeta =
		cache.library().get<core::meta::shader>(UID::convert("4429d63a-9867-468f-a03f-cf56fee3c82e")).value();

	// create the material buffer and instance buffer
	auto matBufferData = create<data::buffer>(cache);
	matBufferData.load(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
					   vk::MemoryPropertyFlagBits::eDeviceLocal,
					   memory::region{1024 * 1024, context_handle->properties().limits.minStorageBufferOffsetAlignment,
									  new memory::default_allocator(false)});
	auto matBuffer = create<gfx::buffer>(cache);
	matBuffer.load(context_handle, matBufferData, stagingBuffer);

	// create the texture
	auto textureHandle = create<gfx::texture>(cache, UID::convert("3c4af7eb-289e-440d-99d9-20b5738f0200"));
	textureHandle.load(context_handle);

	// create the sampler
	auto samplerData = create<data::sampler>(cache);
	samplerData.load();
	auto samplerHandle = create<gfx::sampler>(cache);
	samplerHandle.load(context_handle, samplerData);

	// create a pipeline cache
	auto pipeline_cache = create<core::gfx::pipeline_cache>(cache);
	pipeline_cache.load(context_handle);

	// load the example material
	auto matData = create<data::material>(cache);
	matData.load();
	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});
	auto stages = matData->stages();

	for(auto& stage : stages)
	{
		if(stage.shader_stage() != vk::ShaderStageFlagBits::eFragment) continue;

		auto bindings = stage.bindings();
		bindings[0].texture(textureHandle.RUID());
		bindings[0].sampler(samplerHandle.RUID());
		stage.bindings(bindings);
		// binding.texture()
	}
	matData->stages(stages);
	serialization::serializer s;
	s.serialize<serialization::encode_to_format>(*(data::material*)matData.cvalue(),
												 utility::application::path::get_path() + "material_example.mat");

	auto material = create<gfx::material>(cache);
	material.load(context_handle, matData, pipeline_cache, matBuffer, matBuffer);

	// create the draw instruction
	core::gfx::drawgroup dGroup{};

	auto& default_layer = dGroup.layer("default", 0);
	auto& call			= dGroup.add(default_layer, material);
	call.add(geometry);

	pass.add(dGroup);

	world world{};
	camera& cam = world.add<camera>(frameBuffer, surface_handle);
	pass.build();

	transform_inputlistener listener{cam.transform(), surface_handle->input()};

	uint64_t frameCount										 = 0u;
	std::chrono::high_resolution_clock::time_point last_tick = std::chrono::high_resolution_clock::now();
	while(surface_handle->tick())
	{
		if(surface_handle->open() && swapchain_handle->is_ready())
		{
			pass.prepare();
			pass.present();
		}
		auto current_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsed =
			std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_tick);
		last_tick = current_time;
		world.tick(elapsed);
		listener.tick(elapsed);
		++frameCount;
	}
	return 0;
}

static bool initialized = false;
#if defined(PLATFORM_ANDROID)

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
int main() { return entry(); }
#endif

#endif
#endif