
// core.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS
// #include <Windows.h>
// #include "stdafx.h"
#include "core/resource/resource.hpp"
#include "psl/application_utils.hpp"
#include "psl/library.hpp"
#include "psl/platform_utils.hpp"

#include "core/paradigm.hpp"

#include "core/logging.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#ifdef _MSC_VER
	#include "spdlog/sinks/msvc_sink.h"
#endif
#include "core/gfx/limits.hpp"
#include "core/gfx/types.hpp"
#include "core/utility/geometry.hpp"

#include "core/data/buffer.hpp"
#include "core/data/geometry.hpp"
#include "core/data/material.hpp"
#include "core/data/sampler.hpp"
#include "core/data/window.hpp"	   // application data

#include "core/os/context.hpp"
#include "core/os/surface.hpp"	  // the OS surface to draw one

#include "core/meta/shader.hpp"
#include "core/meta/texture.hpp"

#include "core/gfx/buffer.hpp"
#include "core/gfx/compute.hpp"
#include "core/gfx/context.hpp"
#include "core/gfx/framebuffer.hpp"
#include "core/gfx/geometry.hpp"
#include "core/gfx/material.hpp"
#include "core/gfx/pipeline_cache.hpp"
#include "core/gfx/sampler.hpp"
#include "core/gfx/shader.hpp"
#include "core/gfx/swapchain.hpp"
#include "core/gfx/texture.hpp"

#include "core/gfx/bundle.hpp"
#include "core/gfx/computecall.hpp"
#include "core/gfx/computepass.hpp"
#include "core/gfx/drawpass.hpp"
#include "core/gfx/render_graph.hpp"

#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/dead_tag.hpp"
#include "core/ecs/components/input_tag.hpp"
#include "core/ecs/components/lifetime.hpp"
#include "core/ecs/components/renderable.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/ecs/components/velocity.hpp"
#include "psl/ecs/state.hpp"

#include "core/ecs/systems/attractor.hpp"
#include "core/ecs/systems/death.hpp"
#include "core/ecs/systems/fly.hpp"
#include "core/ecs/systems/geometry_instance.hpp"
#include "core/ecs/systems/gpu_camera.hpp"
#include "core/ecs/systems/lifetime.hpp"
#include "core/ecs/systems/lighting.hpp"
#include "core/ecs/systems/movement.hpp"
#include "core/ecs/systems/render.hpp"
#include "core/ecs/systems/text.hpp"

#include "psl/literals.hpp"

#include "core/data/framebuffer.hpp"

#if defined(PE_PLATFORM_ANDROID)
	#include <android_native_app_glue.h>
#endif
using namespace core;
using namespace core::resource;
using namespace core::gfx;

using namespace psl::ecs;
using namespace core::ecs::components;

handle<core::gfx::compute> create_compute(resource::cache_t& cache,
										  handle<core::gfx::context> context_handle,
										  handle<core::gfx::pipeline_cache> pipeline_cache,
										  const psl::UID& shader,
										  const psl::UID& texture) {
	auto meta = cache.library().get<core::meta::shader>(shader).value();
	auto data = cache.create<data::material_t>();
	data->from_shaders(cache.library(), {meta});
	auto stages = data->stages();
	for(auto& stage : stages) {
		auto bindings = stage.bindings();
		bindings[0].texture(texture);
		stage.bindings(bindings);
	}
	data->stages(stages);
	return cache.instantiate<core::gfx::compute>(
	  "594b2b8a-d4ea-e162-2b2c-987de571c7be"_uid, context_handle, data, pipeline_cache);
}

void load_texture(resource::cache_t& cache, handle<core::gfx::context> context_handle, const psl::UID& texture) {
	if(!cache.contains(texture)) {
		auto textureHandle = cache.instantiate<gfx::texture_t>(texture, context_handle);
		psl_assert(textureHandle, "invalid textureHandle");
	}
}
handle<core::data::material_t> setup_gfx_material_data(resource::cache_t& cache,
													   handle<core::gfx::context> context_handle,
													   psl::UID vert,
													   psl::UID frag,
													   psl::array_view<psl::UID> textures = {})

{
	core::meta::shader obj {};
	auto vertShaderMeta = cache.library().get<core::meta::shader>(vert).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>(frag).value();

	std::for_each(std::begin(textures), std::end(textures), [&cache, &context_handle](const auto& uid) {
		load_texture(cache, context_handle, uid);
	});

	// create the sampler
	auto samplerData   = cache.create<data::sampler_t>();
	auto samplerHandle = cache.create<gfx::sampler_t>(context_handle, samplerData);

	// load the example material
	auto matData = cache.create<data::material_t>();

	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});

	auto texture_it = std::begin(textures);
	auto stages		= matData->stages();
	for(auto& stage : stages) {
		if(stage.shader_stage() != core::gfx::shader_stage::fragment || texture_it == std::end(textures))
			continue;

		auto bindings = stage.bindings();
		for(auto& binding : bindings) {
			if(binding.descriptor() != core::gfx::binding_type::combined_image_sampler)
				continue;
			binding.texture(*texture_it);
			binding.sampler(samplerHandle);
			texture_it = std::next(texture_it);
		}
		stage.bindings(bindings);
	}
	matData->stages(stages);

	matData->blend_states({core::data::material_t::blendstate(0)});
	return matData;
}

handle<core::gfx::material_t> setup_gfx_material(resource::cache_t& cache,
												 handle<core::gfx::context> context_handle,
												 handle<core::gfx::pipeline_cache> pipeline_cache,
												 handle<core::gfx::buffer_t> matBuffer,
												 psl::UID vert,
												 psl::UID frag,
												 psl::array_view<psl::UID> textures = {}) {
	auto matData  = setup_gfx_material_data(cache, context_handle, vert, frag, textures);
	auto material = cache.create<core::gfx::material_t>(context_handle, matData, pipeline_cache, matBuffer);

	return material;
}


handle<core::gfx::material_t> setup_gfx_depth_material(resource::cache_t& cache,
													   handle<core::gfx::context> context_handle,
													   handle<core::gfx::pipeline_cache> pipeline_cache,
													   handle<core::gfx::buffer_t> matBuffer) {
	auto vertShaderMeta = cache.library().get<core::meta::shader>("954b4ef3-f9ec-6a64-a127-ff37a9b31595"_uid).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>("5340928c-5109-3688-cd5a-161766082a9c"_uid).value();


	auto matData = cache.create<data::material_t>();

	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});

	auto material = cache.create<gfx::material_t>(context_handle, matData, pipeline_cache, matBuffer);
	return material;
}

void create_ui(psl::ecs::state_t& state) {}

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

void setup_loggers() {
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c						  = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm							  = localtime_safe(now_c);
	psl::string time;
	time.resize(20);
	strftime(time.data(), 20, "%Y-%m-%d %H-%M-%S", &now_tm);
	time[time.size() - 1] = '/';
	psl::string sub_path  = "logs/" + time;
	if(!psl::utility::platform::file::exists(psl::utility::application::path::get_path() + sub_path + "main.log"))
		psl::utility::platform::file::write(psl::utility::application::path::get_path() + sub_path + "main.log", "");
	std::vector<spdlog::sink_ptr> sinks;

	auto mainlogger = std::make_shared<spdlog::sinks::dist_sink_mt>();
	mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "main.log", true));
	mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + "logs/latest.log", true));
	#ifdef _MSC_VER
	mainlogger->add_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>());
	#else
	auto outlogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	outlogger->set_level(spdlog::level::level_enum::warn);
	mainlogger->add_sink(outlogger);
	#endif

	auto ivklogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "ivk.log", true);

	auto igleslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "igles.log", true);

	auto gfxlogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "gfx.log", true);

	auto systemslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "systems.log", true);

	auto oslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "os.log", true);

	auto datalogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "data.log", true);

	auto corelogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "core.log", true);

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
void setup_loggers() {
	core::log		   = spdlog::android_logger_mt("main", "paradigm");
	core::systems::log = spdlog::android_logger_mt("systems", "paradigm");
	core::os::log	   = spdlog::android_logger_mt("os", "paradigm");
	core::data::log	   = spdlog::android_logger_mt("data", "paradigm");
	core::gfx::log	   = spdlog::android_logger_mt("gfx", "paradigm");
	core::ivk::log	   = spdlog::android_logger_mt("ivk", "paradigm");
	spdlog::set_pattern("[%8T:%6f] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}

#endif

auto generate_fullscreen_quad(core::resource::cache_t& cache,
							  core::resource::handle<core::gfx::context>& context_handle,
							  core::resource::handle<core::gfx::buffer_t>& vertexBuffer,
							  core::resource::handle<core::gfx::buffer_t>& indexBuffer) {
	resource::handle<data::geometry_t> geometryDataHandle {core::utility::geometry::create_quad(cache, 1, -1, -1, 1)};
	core::utility::geometry::set_channel(geometryDataHandle, core::data::geometry_t::constants::COLOR, psl::vec4::one);
	return cache.create<gfx::geometry_t>(context_handle, geometryDataHandle, vertexBuffer, indexBuffer);
}

auto generate_test_geometry(core::resource::cache_t& cache,
							core::resource::handle<core::gfx::context>& context_handle,
							core::resource::handle<core::gfx::buffer_t>& vertexBuffer,
							core::resource::handle<core::gfx::buffer_t>& indexBuffer)
  -> std::vector<resource::handle<gfx::geometry_t>> {
	std::vector<resource::handle<data::geometry_t>> geometryDataHandles;
	std::vector<resource::handle<gfx::geometry_t>> geometryHandles;
	geometryDataHandles.push_back(core::utility::geometry::create_icosphere(cache, psl::vec3::one, 0));
	geometryDataHandles.push_back(core::utility::geometry::create_cone(cache, 1.0f, 1.0f, 1.0f, 12));
	geometryDataHandles.push_back(core::utility::geometry::create_spherified_cube(cache, psl::vec3::one, 2));
	geometryDataHandles.push_back(core::utility::geometry::create_box(cache, psl::vec3::one));
	geometryDataHandles.push_back(core::utility::geometry::create_sphere(cache, psl::vec3::one, 12, 8));
	// up
	geometryDataHandles.push_back(core::utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::back * 90.0f));
	core::utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::up * 0.5f);
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::forward * 90.0f),
									core::data::geometry_t::constants::NORMAL);
	auto up_plane_index = geometryDataHandles.size() - 1;
	// down
	geometryDataHandles.push_back(core::utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::forward * 90.0f));
	core::utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::down * 0.5f);
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::back * 90.0f),
									core::data::geometry_t::constants::NORMAL);
	auto down_plane_index = geometryDataHandles.size() - 1;
	// left
	geometryDataHandles.push_back(core::utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::down * 90.0f));
	core::utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::left * 0.5f);
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::up * 90.0f),
									core::data::geometry_t::constants::NORMAL);
	auto left_plane_index = geometryDataHandles.size() - 1;
	// right
	geometryDataHandles.push_back(core::utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::up * 90.0f));
	core::utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::right * 0.5f);
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::down * 90.0f),
									core::data::geometry_t::constants::NORMAL);
	auto right_plane_index = geometryDataHandles.size() - 1;
	// forward
	geometryDataHandles.push_back(core::utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	core::utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::forward * 0.5f);
	auto forward_plane_index = geometryDataHandles.size() - 1;
	// back
	geometryDataHandles.push_back(core::utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::left * 90.0f));
	core::utility::geometry::translate(geometryDataHandles[geometryDataHandles.size() - 1], psl::vec3::back * 0.5f);
	core::utility::geometry::rotate(geometryDataHandles[geometryDataHandles.size() - 1],
									psl::math::from_euler(psl::vec3::right * 90.0f),
									core::data::geometry_t::constants::NORMAL);
	auto back_plane_index = geometryDataHandles.size() - 1;

	geometryDataHandles.push_back(
	  core::utility::geometry::create_plane(cache, psl::vec2::one * 128.f, psl::ivec2::one, psl::vec2::one * 8.f));
	geometryDataHandles.push_back(core::utility::geometry::create_icosphere(cache, psl::vec3::one, 4));

	// geometryDataHandles.push_back(
	//   cache.instantiate<core::data::geometry_t>("bf36d6f1-af53-41b9-b7ae-0f0cb16d8734"_uid));
	auto water_plane_index = geometryDataHandles.size() - 1;
	for(auto& handle : geometryDataHandles) {
		if(!handle->contains(core::data::geometry_t::constants::COLOR)) {
			core::vertex_stream_t colorstream {core::vertex_stream_t::type::vec3};
			auto& colors = colorstream.get<core::vertex_stream_t::type::vec3>();
			auto& normalStream =
			  handle->vertices(core::data::geometry_t::constants::NORMAL).get<core::vertex_stream_t::type::vec3>();
			colors.resize(normalStream.size());
			std::memcpy(colors.data(), normalStream.data(), sizeof(psl::vec3) * normalStream.size());

			std::for_each(std::begin(colors), std::end(colors), [](auto& color) {
				color =
				  (psl::math::dot(psl::math::normalize(color), psl::math::normalize(psl::vec3(145, 170, 35))) + 1.0f) *
				  0.5f;
				// color = std::max((color[0] + color[1] + color[2]), 0.0f) + 0.33f;
			});
			handle->vertices(core::data::geometry_t::constants::COLOR, colorstream);
			// handle->erase(core::data::geometry_t::constants::TANGENT);
			// handle->erase(core::data::geometry_t::constants::NORMAL);
		}
		geometryHandles.emplace_back(cache.create<gfx::geometry_t>(context_handle, handle, vertexBuffer, indexBuffer));
	}

	return geometryHandles;
}

auto create_fbo_data(core::resource::cache_t& cache,
					 core::resource::handle<core::os::surface>& surface_handle,
					 core::resource::handle<core::gfx::context>& context_handle,
					 bool with_depth = false) {
	auto frameBufferData =
	  cache.create<core::data::framebuffer_t>(surface_handle->data().width(), surface_handle->data().height(), 1);

	{	 // set the sampler state
		auto ppsamplerData = cache.create<data::sampler_t>();
		ppsamplerData->mipmaps(false);
		auto ppsamplerHandle = cache.create<gfx::sampler_t>(context_handle, ppsamplerData);
		frameBufferData->set(ppsamplerHandle);
	}

	{	 // render target
		core::gfx::attachment descr {};
		descr.format		= core::gfx::format_t::r32g32b32a32_sfloat;
		descr.sample_bits	= 1;
		descr.image_load	= core::gfx::attachment::load_op::clear;
		descr.image_store	= core::gfx::attachment::store_op::store;
		descr.stencil_load	= core::gfx::attachment::load_op::dont_care;
		descr.stencil_store = core::gfx::attachment::store_op::dont_care;
		descr.initial		= core::gfx::image::layout::undefined;
		descr.final			= core::gfx::image::layout::general;

		frameBufferData->add(surface_handle->data().width(),
							 surface_handle->data().height(),
							 1,
							 core::gfx::image::usage::color_attachment | core::gfx::image::usage::sampled,
							 core::gfx::clear_value(psl::ivec4 {0}),
							 descr);
	}

	if(with_depth) {	// depth-stencil target
		core::gfx::attachment descr {};
		if(auto format = context_handle->limits().supported_depthformat; format == core::gfx::format_t::undefined) {
			core::log->error("Could not find a suitable depth stencil buffer format.");
		} else
			descr.format = format;
		descr.sample_bits	= 1;
		descr.image_load	= core::gfx::attachment::load_op::clear;
		descr.image_store	= core::gfx::attachment::store_op::dont_care;
		descr.stencil_load	= core::gfx::attachment::load_op::dont_care;
		descr.stencil_store = core::gfx::attachment::store_op::dont_care;
		descr.initial		= core::gfx::image::layout::undefined;
		descr.final			= core::gfx::image::layout::depth_stencil_attachment_optimal;

		frameBufferData->add(surface_handle->data().width(),
							 surface_handle->data().height(),
							 1,
							 core::gfx::image::usage::dept_stencil_attachment,
							 core::gfx::depth_stencil {1.0f, 0},
							 descr);
	}

	return frameBufferData;
}

int entry(gfx::graphics_backend backend, core::os::context& os_context) {
	psl::string libraryPath {psl::utility::application::path::library + "resources.metalib"};

	memory::region resource_region {20_mb, 4u, new memory::default_allocator()};
	psl::string8_t environment = "";
	switch(backend) {
	case graphics_backend::gles:
		environment = "gles";
		break;
	case graphics_backend::vulkan:
		environment = "vulkan";
		break;
	}

	core::log->info("creating cache");
	cache_t cache {psl::meta::library {psl::to_string8_t(libraryPath), {{environment}}}};
	core::log->info("cache created");
	// cache cache{psl::meta::library{psl::to_string8_t(libraryPath), {{environment}}}, resource_region.allocator()};

	auto window_data = cache.instantiate<data::window>("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
	window_data->name(APPLICATION_FULL_NAME + " { " + environment + " }");
	auto surface_handle = cache.create<core::os::surface>(window_data);
	if(!surface_handle) {
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	auto context_handle = cache.create<core::gfx::context>(backend, psl::string8_t {APPLICATION_NAME});

	auto swapchain_handle = cache.create<core::gfx::swapchain>(surface_handle, context_handle, os_context);

	// get a vertex and fragment shader that can be combined, we only need the meta
	if(!cache.library().contains("234318ae-3590-f1e2-bac5-f113cac3be9b"_uid) ||
	   !cache.library().contains("5e43dd8b-d10e-2ea0-a9d6-df4199bbe2aa"_uid)) {
		core::log->critical(
		  "Could not find the required shader resources in the meta library. Did you forget to copy the files over?");
		if(surface_handle)
			surface_handle->terminate();
		return -1;
	}

	auto storage_buffer_align = context_handle->limits().storage.alignment;
	auto uniform_buffer_align = context_handle->limits().uniform.alignment;
	auto mapped_buffer_align  = context_handle->limits().memorymap.alignment;

	// create a staging buffer, this is allows for more advantagous resource access for the GPU
	core::resource::handle<gfx::buffer_t> stagingBuffer {};
	if(backend == graphics_backend::vulkan) {
		auto stagingBufferData = cache.create<data::buffer_t>(
		  core::gfx::memory_usage::transfer_source,
		  core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
		  memory::region {(size_t)128_mb, 4, new memory::default_allocator(false)});
		stagingBuffer = cache.create<gfx::buffer_t>(context_handle, stagingBufferData);
	}

	// create the buffers to store the model in
	// - memory region which we'll use to track the allocations, this is supposed to be virtual as we don't care to
	//   have a copy on the CPU
	// - then we create the vulkan buffer resource to interface with the GPU
	auto vertexBufferData = cache.create<data::buffer_t>(
	  core::gfx::memory_usage::vertex_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {256_mb, 4, new memory::default_allocator(false)});
	auto vertexBuffer = cache.create<gfx::buffer_t>(context_handle, vertexBufferData, stagingBuffer);

	auto indexBufferData = cache.create<data::buffer_t>(
	  core::gfx::memory_usage::index_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {128_mb, 4, new memory::default_allocator(false)});
	auto indexBuffer = cache.create<gfx::buffer_t>(context_handle, indexBufferData, stagingBuffer);

	auto dynamicInstanceBufferData =
	  cache.create<data::buffer_t>(core::gfx::memory_usage::vertex_buffer,
								   core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
								   memory::region {128_mb, 4, new memory::default_allocator(false)});

	// instance buffer for vertex data, these are unique per streamed instance of a geometry in a shader
	auto instanceBufferData = cache.create<data::buffer_t>(
	  core::gfx::memory_usage::vertex_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {128_mb, 4, new memory::default_allocator(false)});
	auto instanceBuffer = cache.create<gfx::buffer_t>(context_handle, instanceBufferData, stagingBuffer);

	// instance buffer for material data, these are shared over all instances of a given material bind (over all
	// instances in the invocation)
	auto instanceMaterialBufferData = cache.create<data::buffer_t>(
	  core::gfx::memory_usage::uniform_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {8_mb, uniform_buffer_align, new memory::default_allocator(false)});
	auto instanceMaterialBuffer =
	  cache.create<gfx::buffer_t>(context_handle, instanceMaterialBufferData, stagingBuffer);
	auto intanceMaterialBinding = cache.create<gfx::shader_buffer_binding>(instanceMaterialBuffer, 8_mb);
	cache.library().set(intanceMaterialBinding.uid(), core::data::material_t::MATERIAL_DATA);

	std::vector<resource::handle<gfx::geometry_t>> geometryHandles {
	  generate_test_geometry(cache, context_handle, vertexBuffer, indexBuffer)};

	auto fullscreenQuad = generate_fullscreen_quad(cache, context_handle, vertexBuffer, indexBuffer);

	// create the buffer that we'll use for storing the WVP for the shaders;
	auto globalShaderBufferData = cache.create<data::buffer_t>(
	  core::gfx::memory_usage::uniform_buffer,
	  core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
	  resource_region.create_region(1_mb, uniform_buffer_align, new memory::default_allocator(true)).value());

	auto globalShaderBuffer	   = cache.create<gfx::buffer_t>(context_handle, globalShaderBufferData);
	auto frameCamBufferBinding = cache.create<gfx::shader_buffer_binding>(
	  globalShaderBuffer, 100_kb, sizeof(core::ecs::systems::gpu_camera::framedata));
	cache.library().set(frameCamBufferBinding, "GLOBAL_DYNAMIC_WORLD_VIEW_PROJECTION_MATRIX");


	// create a pipeline cache
	auto pipeline_cache = cache.create<core::gfx::pipeline_cache>(context_handle);

	psl::array<core::resource::handle<core::gfx::material_t>> materials;
	core::resource::handle<core::gfx::material_t> depth_material =
	  setup_gfx_depth_material(cache, context_handle, pipeline_cache, instanceMaterialBuffer);

	// water
	materials.emplace_back(setup_gfx_material(cache,
											  context_handle,
											  pipeline_cache,
											  instanceMaterialBuffer,
											  "0f48f21f-f707-06b5-5c66-83ff0d53c5a1"_uid,
											  "b942da62-2922-c985-9c02-ae3008f7a8bc"_uid/*,
											  "9b42f9b6-75f6-a4f1-a219-986033d37d8a"_uid*/));

	// // grass
	// materials.emplace_back(setup_gfx_material(cache,
	// 										  context_handle,
	// 										  pipeline_cache,
	// 										  instanceMaterialBuffer,
	// 										  "0f48f21f-f707-06b5-5c66-83ff0d53c5a1"_uid,
	// 										  "b942da62-2922-c985-9c02-ae3008f7a8bc"_uid,
	// 										  "944e7173-ede1-0bed-cffe-d6a5a34449be"_uid));

	// // dirt
	// materials.emplace_back(setup_gfx_material(cache,
	// 										  context_handle,
	// 										  pipeline_cache,
	// 										  instanceMaterialBuffer,
	// 										  "0f48f21f-f707-06b5-5c66-83ff0d53c5a1"_uid,
	// 										  "b942da62-2922-c985-9c02-ae3008f7a8bc"_uid,
	// 										  "f24fa9d7-966a-e942-851b-5b6fb30dd0b6"_uid));

	// // rock
	// materials.emplace_back(setup_gfx_material(cache,
	// 										  context_handle,
	// 										  pipeline_cache,
	// 										  instanceMaterialBuffer,
	// 										  "0f48f21f-f707-06b5-5c66-83ff0d53c5a1"_uid,
	// 										  "b942da62-2922-c985-9c02-ae3008f7a8bc"_uid,
	// 										  "eb4e6f57-1d5d-56d3-41ed-27ea6b5f5ea1"_uid));

	psl::array<core::resource::handle<core::gfx::bundle>> bundles;
	bundles.emplace_back(cache.create<gfx::bundle>(instanceBuffer, intanceMaterialBinding));
	bundles.back()->set_material(materials[0], 2000);
	bundles.back()->set_material(depth_material, 1000);

	// bundles.emplace_back(cache.create<gfx::bundle>(instanceBuffer, intanceMaterialBinding));
	// bundles.back()->set_material(materials[1], 2000);
	// bundles.back()->set_material(depth_material, 1000);

	// bundles.emplace_back(cache.create<gfx::bundle>(instanceBuffer, intanceMaterialBinding));
	// bundles.back()->set_material(materials[2], 2000);

	// bundles.emplace_back(cache.create<gfx::bundle>(instanceBuffer, intanceMaterialBinding));
	// bundles.back()->set_material(materials[3], 2000);

	core::gfx::render_graph renderGraph {};
	auto geometryFBO = cache.create<core::gfx::framebuffer_t>(
	  context_handle, create_fbo_data(cache, surface_handle, context_handle, true));

	core::resource::handle<core::gfx::bundle> post_effect_bundle =
	  cache.create<gfx::bundle>(instanceBuffer, intanceMaterialBinding);

	auto post_effect_data = setup_gfx_material_data(cache,
													context_handle,
													"0b4cb8ca-b3d0-d105-c7be-8ed4eb5f3395"_uid,
													"cc4889f4-bbd6-65ae-3c2b-758a8e7b5bbf"_uid,
													std::vector {geometryFBO->texture(0).meta().ID()});
	// post_effect_data->blend_states({core::data::material_t::blendstate::transparent(0)});
	post_effect_data->cull_mode(core::gfx::cullmode::none);
	post_effect_data->depth_write(false);
	post_effect_data->depth_test(false);
	post_effect_data->depth_compare_op(core::gfx::compare_op::always);
	auto post_effect_material =
	  cache.create<core::gfx::material_t>(context_handle, post_effect_data, pipeline_cache, instanceMaterialBuffer);
	post_effect_bundle->set_material(post_effect_material, 4000);
	post_effect_bundle->set("color", psl::vec4::one);
	// auto fbo_texture = geometryFBO->texture(0);

	/*
	core::resource::handle<core::gfx::bundle> atmos_effect_bundle =
		cache.create<gfx::bundle>(instanceBuffer, intanceMaterialBinding);
	auto atmos_effect_data = setup_gfx_material_data(cache, context_handle, "ad493acc-9b2f-2bce-ec98-d5df87242298"_uid,
													 "d44d9a5e-095e-9f15-2682-aae04c562790"_uid);
	atmos_effect_data->blend_states({core::data::material_t::blendstate::opaque(0)});
	atmos_effect_data->cull_mode(core::gfx::cullmode::none);
	auto atmos_effect_material =
		cache.create<core::gfx::material_t>(context_handle, atmos_effect_data, pipeline_cache, instanceMaterialBuffer);
	atmos_effect_bundle->set_material(atmos_effect_material, 5000);
	atmos_effect_bundle->set("lightDir", psl::vec4::one);
	// atmos_effect_bundle->set("OuterRadius", 102.5f);
	*/
	auto geometry_pass	= renderGraph.create_drawpass(context_handle, geometryFBO);
	auto swapchain_pass = renderGraph.create_drawpass(context_handle, swapchain_handle);

	renderGraph.connect(geometry_pass, swapchain_pass);
#ifdef COMPUTE
	{
		psl::UID compute_texture_uid;
		std::unique_ptr<core::meta::texture_t> texture = std::make_unique<core::meta::texture_t>();
		texture->width(512);
		texture->height(512);
		texture->format(core::gfx::format_t::r8g8b8a8_unorm);
		auto handle			= cache.create_using<core::gfx::texture_t>(std::move(texture), context_handle);
		compute_texture_uid = handle.meta()->ID();

		auto compute_handle = create_compute(
		  cache, context_handle, pipeline_cache, "9d48b49f-d82f-bcbd-a180-93a713e43b98"_uid, compute_texture_uid);

		if(context_handle->backend() == core::gfx::graphics_backend::gles) {
			auto compute_pass = renderGraph.create_computepass(context_handle);
			compute_pass->add(core::gfx::computecall {compute_handle});
			renderGraph.connect(compute_pass, swapchain_pass);
		}
	}
#endif
	// create the ecs
	using psl::ecs::state_t;

	state_t ECSState {1u};

	using namespace core::ecs::components;

	const size_t area			  = 128;
	const size_t area_granularity = 128;
	const size_t size_steps		  = 24;
	const float timeScale		  = 1.f;
	const size_t spawnInterval	  = (size_t)((float)100 * (1.f / timeScale));

	psl::utility::platform::file::write(psl::utility::application::path::get_path() + "frame_data.txt",
										core::profiler.to_string());


	core::ecs::components::transform camTrans {psl::vec3 {40, 15, 150}};
	camTrans.rotation = psl::math::look_at_q(camTrans.position, psl::vec3::zero, psl::vec3::up);


	core::ecs::systems::render render_system {ECSState, geometry_pass};
	render_system.add_render_range(2000, 3000);
	core::ecs::systems::render post_render_system {ECSState, swapchain_pass};
	post_render_system.add_render_range(4000, 6000);
	core::ecs::systems::fly fly_system {ECSState, surface_handle->input()};
	core::ecs::systems::gpu_camera gpu_camera_system {
	  ECSState, surface_handle, frameCamBufferBinding, context_handle->backend()};

	ECSState.declare<"movement">(psl::ecs::threading::par, core::ecs::systems::movement);
	ECSState.declare<"lifetime">(psl::ecs::threading::par, core::ecs::systems::lifetime);
	ECSState.declare<"downscale">(psl::ecs::threading::par,
								  [](info_t& info, pack_direct_full_t<transform, const lifetime> pack) {
									  for(auto [transf, life] : pack) {
										  auto remainder = std::min(life.value * 2.0f, 1.0f);
										  transf.scale *= remainder;
									  }
								  });

	ECSState.declare<"attractor">(psl::ecs::threading::par, core::ecs::systems::attractor);
	core::ecs::systems::geometry_instancing geometry_instancing_system {ECSState};

	core::ecs::systems::lighting_system lighting {psl::view_ptr(&ECSState),
												  psl::view_ptr(&cache),
												  resource_region,
												  psl::view_ptr(&renderGraph),
												  swapchain_pass,
												  context_handle,
												  surface_handle};

	/*core::ecs::systems::text text{ECSState,	   cache,		   context_handle, vertexBuffer,
								  indexBuffer, pipeline_cache, matBuffer,	   instanceBuffer,
	   instanceMaterialBuffer};*/

	auto eCam = ECSState.create(1,
								std::move(camTrans),
								psl::ecs::empty<core::ecs::components::camera> {},
								psl::ecs::empty<core::ecs::components::input_tag> {});

	size_t iterations										  = 25600;
	std::chrono::high_resolution_clock::time_point last_tick  = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point next_spawn = std::chrono::high_resolution_clock::now();

	// fullscreen quad entity
	ECSState.create(
	  1,
	  [&post_effect_bundle, &geometry = fullscreenQuad](core::ecs::components::renderable& renderable) {
		  renderable = {post_effect_bundle, geometry};
	  },
	  core::ecs::components::transform {{0, 0, 1.f}});

	/*
// atmospheric effect
ECSState.create(
	1,
	[&atmos_effect_bundle,
	 &geometry = geometryHandles[fullscreen_quad_index]](core::ecs::components::renderable& renderable) {
		renderable = {atmos_effect_bundle, geometry};
	},
	core::ecs::components::transform{});
	*/
	if(false) {
		load_texture(cache, context_handle, "5ea8ae3d-1ff4-48cc-9c90-d0eb81ba7075"_uid);
		auto water_material_data = setup_gfx_material_data(cache,
														   context_handle,
														   "b64676ca-7000-08d2-e2ef-48c25742a6bc"_uid,
														   "7246dfbd-29f6-65db-c89e-1b42036b368b"_uid,
														   std::vector {"3c4af7eb-289e-440d-99d9-20b5738f0200"_uid});
		auto stages				 = water_material_data->stages();
		auto bindings			 = stages[1].bindings();
		bindings[2].texture("5ea8ae3d-1ff4-48cc-9c90-d0eb81ba7075"_uid);

		stages[1].bindings(bindings);
		water_material_data->stages(stages);
		water_material_data->cull_mode(core::gfx::cullmode::front);
		auto water_material = cache.create<core::gfx::material_t>(
		  context_handle, water_material_data, pipeline_cache, instanceMaterialBuffer);

		bundles.emplace_back(cache.create<gfx::bundle>(instanceBuffer, intanceMaterialBinding));
		bundles.back()->set_material(water_material, 2000);
		bundles.back()->set("color", psl::vec4 {1.f, 1.f, 1.f, 0.5f});
		bundles.back()->set("lightDir", psl::vec4 {1.f, 1.f, 1.f, 0.f});
		ECSState.create(
		  1,
		  [&bundle	 = bundles.back(),
		   &geometry = geometryHandles[/*water_plane_index*/ 0]](core::ecs::components::renderable& renderable) {
			  renderable = {bundle, geometry};
		  },
		  core::ecs::components::transform {psl::vec3 {}, psl::vec3::one * 1.f});
	}

	ECSState.create(1,
					psl::ecs::empty<core::ecs::components::transform> {},
					core::ecs::components::light {{}, 1.0f, core::ecs::components::light::type::DIRECTIONAL, true});

	std::array<int, 4> matusage {0};

	std::string str {"dsff"};
	auto textEntity = ECSState.create(1,
									  core::ecs::components::text {&str},
									  core::ecs::components::transform {{}, psl::vec3::one * 3.0f},
									  psl::ecs::empty<core::ecs::components::dynamic_tag>());

	size_t frame {0};
	std::chrono::duration<float> dTime {};
	std::chrono::duration<float> elapsed {};

#ifdef PE_DEBUG
	size_t count = 5;
	size_t swing = 0;
	size_t burst = 1500;
#else
	size_t count = 750;
	size_t swing = 100;
	size_t burst = 40000;
#endif

	while(os_context.tick() && surface_handle->tick()) {
		core::log->info("---- FRAME {0} START ----", frame);
		core::log->info("There are {} renderables alive right now", ECSState.size<renderable>());
		core::profiler.next_frame();

		core::profiler.scope_begin("system tick");
		ECSState.tick(dTime * timeScale);
		core::profiler.scope_end();

		core::profiler.scope_begin("presenting");
		renderGraph.present();
		core::profiler.scope_end();

		core::profiler.scope_begin("creating entities");


		str = "material 0: " + std::to_string(matusage[0]) + " material 1: " + std::to_string(matusage[1]) +
			  " material 2: " + std::to_string(matusage[2]) + " material 3: " + std::to_string(matusage[3]);
		// ECSState.set_component(textEntity, core::ecs::components::text{ &str });

		core::profiler.scope_end();

		// static_assert(psl::ecs::details::key_for<float>() != psl::ecs::details::key_for<lifetime>());

		auto current_time = std::chrono::high_resolution_clock::now();
		dTime			  = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_tick);
		elapsed += dTime;
		// while (current_time >= next_spawn)
		{
			next_spawn += std::chrono::milliseconds(spawnInterval);
			ECSState.create(
			  /*(iterations > 0) ? count + std::rand() % (swing + 1) : 0*/ static_cast<entity_t::size_type>(
				(frame % 250 == 0) ? burst : 0),
			  [&bundles, &geometryHandles, &matusage](core::ecs::components::renderable& renderable) {
				  auto matIndex = 0;
				  // (std::rand() % 2 == 0);
				  matusage[matIndex] += 1;
				  renderable = {bundles[matIndex], geometryHandles[/*std::rand() % geometryHandles.size()*/ 0]};
			  },
			  psl::ecs::empty<core::ecs::components::dynamic_tag> {},
			  psl::ecs::empty<core::ecs::components::transform> {},
			  [](core::ecs::components::lifetime& target) { target = {2.5f + ((std::rand() % 50) / 50.0f) * 5.0f}; },
			  [&size_steps](core::ecs::components::velocity& target) {
				  target = {
					psl::math::normalize(psl::vec3((float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
												   (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
												   (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f)),
					((std::rand() % 5000) / 500.0f) * 8.0f,
					1.0f};
			  });


			if(iterations > 0) {
				if(ECSState.filter<core::ecs::components::attractor>().size() < 1) {
					ECSState.create(
					  2,
					  [](core::ecs::components::lifetime& target) {
						  target = {5.0f + ((std::rand() % 50) / 50.0f) * 5.0f};
					  },
					  [&size_steps](core::ecs::components::attractor& target) {
						  target = {(float)(std::rand() % size_steps) / size_steps * 3 + 0.5f,
									(float)(std::rand() % size_steps) / size_steps * 80};
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
									  (float)(std::rand() % size_steps) / size_steps)};
					  });
				}
				--iterations;
			}
		}

		using namespace psl::math;
		using namespace psl;

		psl::vec3 eulerDir;
		eulerDir[0] = sin(elapsed.count() * 0.1f);
		eulerDir[1] = cos(elapsed.count() * 0.1f);
		eulerDir[2] = 0.3f;
		// lightDir = from_euler(eulerDir + psl::vec3::right);
		// atmos_effect_bundle->set("lightDir", psl::vec4(eulerDir, 0.0f));

		last_tick = current_time;
		core::log->info("---- FRAME {0} END   ---- duration {1} ms", frame++, dTime.count() * 1000);
	}
	context_handle->wait_idle();

	return 0;
}

#if defined(PE_PLATFORM_ANDROID)

// todo move to os context
AAssetManager* psl::utility::platform::file::ANDROID_ASSET_MANAGER = nullptr;

void android_main(android_app* application) {
	auto os_context										= core::os::context {application};
	psl::utility::platform::file::ANDROID_ASSET_MANAGER = application->activity->assetManager;
	setup_loggers();
	std::srand(0);

	// go into a holding loop while wait for the window to come online.
	while(true) {
		os_context.tick();
		__android_log_write(
		  ANDROID_LOG_INFO,
		  "paradigm",
		  (std::string("window ") + std::to_string((size_t)(os_context.application().window))).data());
		if(os_context.application().window != nullptr)
			break;
	}
	__android_log_write(
	  ANDROID_LOG_INFO,
	  "paradigm",
	  (std::string("window is loaded ") + std::to_string((size_t)(os_context.application().window))).data());
	entry(graphics_backend::vulkan, os_context);
	return;
}

#else
int main(int argc, char* argv[]) {
	#ifdef PE_PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	#endif
	setup_loggers();

	#ifdef _MSC_VER
	{	 // here to trick the compiler into generating these types to get UUID natvis support
		dummy::hex_dummy_high hex_dummy_high {};
		dummy::hex_dummy_low hex_dummy_lowy {};
	}
	#endif
	std::srand(0);
	if(argc > 0) {
		core::log->info("Received the cli args:");
		for(auto i = 0; i < argc; ++i) core::log->info(argv[i]);
	}
	auto backend = [](int argc, char* argv[]) {
		for(auto i = 0; i < argc; ++i) {
			std::string_view text {argv[i]};
			if(text == "--vulkan") {
	#if defined(PE_VULKAN)
				return graphics_backend::vulkan;
	#else
				throw std::runtime_error("Requested a Vulkan backend, but application does not support Vulkan");
	#endif
			} else if(text == "--gles") {
	#if defined(PE_GLES)
				return graphics_backend::gles;
	#else
				throw std::runtime_error("Requested a GLES backend, but application does not support GLES");
	#endif
			}
		}
	#if defined(PE_VULKAN)
		return graphics_backend::vulkan;
	#elif defined(PE_GLES)
		return graphics_backend::gles;
	#endif
	}(argc, argv);
	core::os::context context {};
	return entry(backend, context);
}
#endif
