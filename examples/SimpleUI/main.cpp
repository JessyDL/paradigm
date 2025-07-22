

// This example demonstrates creating a simple UI, with control buttons and widgets.
// Do note that this example will not fully explain every concept, it is assumed you have a basic understanding of
// certain rendering concepts, and some knowledge of threading and ECS.


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "core/data/geometry.hpp"
#include "core/data/material.hpp"
#include "core/data/sampler.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/renderable.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/ecs/systems/geometry_instance.hpp"
#include "core/ecs/systems/gpu_camera.hpp"
#include "core/ecs/systems/render.hpp"
#include "core/gfx/geometry.hpp"
#include "core/gfx/material.hpp"
#include "core/gfx/pipeline_cache.hpp"
#include "core/gfx/sampler.hpp"
#include "core/gfx/shader.hpp"
#include "core/gfx/swapchain.hpp"
#include "core/gfx/texture.hpp"
#include "core/gfx/types.hpp"
#include "core/logging.hpp"
#include "core/meta/shader.hpp"
#include "core/meta/texture.hpp"
#include "core/os/context.hpp"
#include "core/resource/resource.hpp"

#include "examples/cli_parser.hpp"
#include "examples/engine_instance.hpp"

int entry(core::gfx::graphics_backend backend, std::unique_ptr<core::os::context> os_context) {
	engine_instance_t::options_t options {};
	options.backend			 = backend;
	options.application_name = "SimpleUI Example";
	engine_instance_t engine_instance {options, std::move(os_context)};

	auto triangleGeomData = engine_instance.cache().create<core::data::geometry_t>();
	{
		core::vertex_stream_t vertexStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t colorStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		auto& vertices = vertexStream.get<core::vertex_stream_t::type::vec3>();
		auto& colors   = colorStream.get<core::vertex_stream_t::type::vec3>();
		auto& uvs	   = uvStream.get<core::vertex_stream_t::type::vec2>();

		vertices.emplace_back(psl::vec3 {-0.5f, -0.5f, 0.0f});
		vertices.emplace_back(psl::vec3 {+0.0f, +0.5f, 0.0f});
		vertices.emplace_back(psl::vec3 {+0.5f, -0.5f, 0.0f});

		colors.emplace_back(psl::vec3 {1.0f, 1.0f, 1.0f});
		colors.emplace_back(psl::vec3 {1.0f, 1.0f, 1.0f});
		colors.emplace_back(psl::vec3 {1.0f, 1.0f, 1.0f});

		uvs.emplace_back(psl::vec2 {0.0f, 0.0f});
		uvs.emplace_back(psl::vec2 {0.5f, 1.0f});
		uvs.emplace_back(psl::vec2 {1.0f, 0.0f});

		triangleGeomData->vertices(core::data::geometry_t::constants::POSITION, vertexStream);
		triangleGeomData->vertices(core::data::geometry_t::constants::COLOR, colorStream);
		triangleGeomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		triangleGeomData->indices(std::vector<uint32_t> {0, 1, 2});
	}
	auto triangleGeometryResource = engine_instance.cache().create<core::gfx::geometry_t>(
	  engine_instance.context(), triangleGeomData, engine_instance.vertex_buffer(), engine_instance.index_buffer());

	auto pipeline_cache = engine_instance.cache().create<core::gfx::pipeline_cache>(engine_instance.context());

	auto const uid_vert_shader = "1a4cf9b4-7328-9094-49db-de4e439fd692"_uid;	// ./shaders/textured.vert.*
	auto const uid_frag_shader = "5d42c614-b31a-9057-331b-2b43bb023f9f"_uid;	// ./shaders/textured.frag.*
	auto vertShaderMeta = engine_instance.cache().library().get<core::meta::shader>(uid_vert_shader).value_or(nullptr);
	auto fragShaderMeta = engine_instance.cache().library().get<core::meta::shader>(uid_frag_shader).value_or(nullptr);

	psl_assert(vertShaderMeta != nullptr && fragShaderMeta != nullptr,
			   "Missing vert/frag shaders. If this happens then you are missing the resources, they should be deployed "
			   "with the binary.");
	auto const uid_material = "5945a26d-c0e0-01a9-ce85-0b6bced962b5"_uid;	 // ./materials/textured.mat

	auto matData = engine_instance.cache().instantiate<core::data::material_t>(uid_material);
	auto material = engine_instance.cache().create<core::gfx::material_t>(
	  engine_instance.context(), matData, pipeline_cache, engine_instance.instance_material_buffer());


	auto bundle = engine_instance.cache().create<core::gfx::bundle>(engine_instance.instance_buffer(),
																	engine_instance.instance_material_binding());
	bundle->set_material(material, 500);

	psl::ecs::state_t state {};
	auto gpuCameraSystem = core::ecs::systems::gpu_camera {
	  state, engine_instance.surface(), engine_instance.frame_cam_buffer_binding(), backend};
	auto geometryInstancingSystem = core::ecs::systems::geometry_instancing {state};
	auto renderSystem			  = core::ecs::systems::render {state, engine_instance.swapchain()};
	renderSystem.add_render_range(0, 1000);

	state.create(
	  1, core::ecs::components::transform {psl::vec3 {0, 0, -2}}, psl::ecs::empty<core::ecs::components::camera> {});

	state.create(1,
				 core::ecs::components::renderable {bundle, triangleGeometryResource},
				 psl::ecs::empty<core::ecs::components::dynamic_tag>(),
				 psl::ecs::empty<core::ecs::components::transform> {});

	engine_instance.run(
	  [&state](auto const& engine_instance, std::chrono::duration<float> dTime) { state.tick(dTime); });

	return 0;
}

int main(int argc, char** argv) {
	core::initialize_loggers();
	if(argc > 0) {
		core::log->info("Received the cli args:");
		for(auto i = 0; i < argc; ++i) core::log->info(argv[i]);
	}

	CLIParser parser {argc, argv};

	// we pick the first available backend for this example.
	// in a proper application you might want to select the best-fit backend based on the
	// application's needs and the system's capabilities.
	// generally though you can assume that modern backends will be able to run the application best for
	// most users while also being the most performant codepath.
	auto backend = parser.get_backends().front();

	return entry(backend, std::make_unique<core::os::context>());
}
