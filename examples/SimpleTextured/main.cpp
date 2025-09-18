

// This example demonstrates creating a simple textured object, it will load everything from data files
// and will let the engine handle loading most of the resources aside from the material and geometry.
// Do note that this example will not fully explain every concept, it is assumed you have a basic understanding of
// certain rendering concepts, and some knowledge of threading and ECS.


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "core/data/geometry.hpp"
#include "core/data/material.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/renderable.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/ecs/systems/gpu_camera.hpp"
#include "core/ecs/systems/render.hpp"
#include "core/gfx/geometry.hpp"
#include "core/gfx/material.hpp"
#include "core/gfx/pipeline_cache.hpp"
#include "core/gfx/types.hpp"
#include "core/logging.hpp"
#include "core/os/context.hpp"
#include "core/resource/resource.hpp"

#include "examples/cli_parser.hpp"
#include "examples/engine_instance.hpp"

int entry(core::gfx::graphics_backend backend, std::unique_ptr<core::os::context> os_context) {
	engine_instance_t::options_t options {};
	options.backend			 = backend;
	options.application_name = "SimpleTextured Example";
	engine_instance_t engine_instance {options, std::move(os_context)};

	// We instantiate a geometry resource from a data file, we use the `instantiate` method so this resource is
	// loaded from disk, and keeps the UID intact (for shared resources).
	auto triangleGeomData =
	  engine_instance.cache().instantiate<core::data::geometry_t>("ea40568b-7009-208b-de85-3168f4b0d1af"_uid);
	auto triangleGeometryResource = engine_instance.cache().create<core::gfx::geometry_t>(
	  engine_instance.context(), triangleGeomData, engine_instance.vertex_buffer(), engine_instance.index_buffer());

	auto pipeline_cache = engine_instance.cache().create<core::gfx::pipeline_cache>(engine_instance.context());

	// The material is also loaded from a data file
	// In this case the material will automatically load the shaders, textures, and samplers referenced in the
	// data file when the material instance (not data) is created.
	auto matData =
	  engine_instance.cache().instantiate<core::data::material_t>("5945a26d-c0e0-01a9-ce85-0b6bced962b5"_uid);
	auto material = engine_instance.cache().create<core::gfx::material_t>(
	  engine_instance.context(), matData, pipeline_cache, engine_instance.instance_material_buffer());

	auto bundle = engine_instance.cache().create<core::gfx::bundle>(engine_instance.instance_buffer(),
																	engine_instance.instance_material_binding());
	bundle->set_material(material, 500);

	psl::ecs::state_t state {};
	auto gpuCameraSystem = core::ecs::systems::gpu_camera {
	  state, engine_instance.surface(), engine_instance.frame_cam_buffer_binding(), backend};
	auto renderSystem = core::ecs::systems::render {state, engine_instance.swapchain()};
	renderSystem.add_render_range(0, 1000);

	state.create(
	  1, core::ecs::components::transform {psl::vec3 {0, 0, -2}}, psl::ecs::empty<core::ecs::components::camera> {});

	state.create(
	  1,
	  [&](core::ecs::components::renderable& renderable) {
		  renderable.bundle		 = bundle;
		  renderable.geometry	 = triangleGeometryResource;
		  renderable.instance_id = bundle->instantiate(triangleGeometryResource, 1).front();
	  },
	  psl::ecs::empty<core::ecs::components::transform_instance_object_model_tag>(),
	  psl::ecs::empty<core::ecs::components::dynamic_tag>(),
	  psl::ecs::empty<core::ecs::components::transform> {});

	engine_instance.run([&state](auto const& engine_instance,
								 std::chrono::duration<float> dTime,
								 std::chrono::duration<float> elapsed) { state.tick(dTime); });

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
