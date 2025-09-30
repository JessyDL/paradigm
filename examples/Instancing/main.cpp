
// This example demonstrated instancing. It will create about 512'000 spheres, most will perish after 14-34 seconds.
// Additionally every frame another 100 spheres will be created at random locations with random velocities that live 2
// seconds.

// The spheres are all instances. We will set the instance data once during creation. It's possible to update it later
// on you can inspect the `core::ecs::systems::render` for that which sets the WVP or transform data every frame to
// update the instances their position.

// Take special note to how we set the instance data in the `core::gfx::bundle` (bulk update), and the
// `core::ecs::components::transform_instance_data_tag` to signify the form of instance data we'll use for the specific
// renderable.

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

#include "core/ecs/systems/fly.hpp"
#include "core/os/surface.hpp"
#include "core/systems/input.hpp"

#include "core/gfx/buffer.hpp"

#include "core/ecs/components/velocity.hpp"
#include "core/ecs/systems/attractor.hpp"
#include "core/ecs/systems/lifetime.hpp"
#include "core/ecs/systems/movement.hpp"

#include "core/utility/geometry.hpp"
#include <queue>

int entry(core::gfx::graphics_backend backend, std::unique_ptr<core::os::context> os_context) {
	engine_instance_t::options_t options {};
	options.instance_buffer.size = 160_mb;	  // make sure we have enough
	options.staging_buffer.size	 = 128_mb;	  // make sure we have enough
	options.backend				 = backend;
	options.application_name	 = "Instancing Example";
	engine_instance_t engine_instance {options, std::move(os_context)};

	auto sphereGeomData	  = core::utility::geometry::create_icosphere(engine_instance.cache(), psl::vec3::one, 2u);
	auto geometryResource = engine_instance.cache().create<core::gfx::geometry_t>(
	  engine_instance.context(), sphereGeomData, engine_instance.vertex_buffer(), engine_instance.index_buffer());


	auto pipeline_cache = engine_instance.cache().create<core::gfx::pipeline_cache>(engine_instance.context());

	auto matData =
	  engine_instance.cache().instantiate<core::data::material_t>("98425890-5e5a-6ba6-fdb0-a3d4641ddea6"_uid);
	auto material = engine_instance.cache().create<core::gfx::material_t>(
	  engine_instance.context(), matData, pipeline_cache, engine_instance.instance_material_buffer());

	auto bundle = engine_instance.cache().create<core::gfx::bundle>(engine_instance.instance_buffer(),
																	engine_instance.instance_material_binding());
	bundle->set_material(material, 500);

	psl::ecs::state_t state {};
	state.declare<"lifetime">(psl::ecs::threading::par, core::ecs::systems::lifetime);
	state.declare<"attractor">(psl::ecs::threading::seq, core::ecs::systems::attractor);
	state.declare<"movement">(psl::ecs::threading::seq, core::ecs::systems::movement);
	core::ecs::systems::fly flySystem(state, engine_instance.surface()->input());

	auto gpuCameraSystem = core::ecs::systems::gpu_camera {
	  state, engine_instance.surface(), engine_instance.frame_cam_buffer_binding(), backend};

	auto renderSystem = core::ecs::systems::render {state, engine_instance.swapchain(), backend};
	renderSystem.add_render_range(0, 1000);

	state.create(1,
				 core::ecs::components::transform {psl::vec3 {0, 0, -2}},
				 psl::ecs::empty<core::ecs::components::camera> {},
				 psl::ecs::empty<core::ecs::components::input_tag> {});

	static const int range_i	  = 80;
	static const float range_f	  = float(range_i);
	static const float midpoint	  = range_f / 2.f;
	static const float bounds_min = midpoint - (midpoint * 9);
	static const float bounds_max = midpoint * 10;

	static const float bounds_range = bounds_max - bounds_min;
	struct bounds_tag {};


	// simple bounds system to keep the spheres from floating away too far
	state.declare<"bounds-check">(
	  [](psl::ecs::pack_indirect_partial_t<const core::ecs::components::transform,
										   core::ecs::components::velocity,
										   psl::ecs::filter<core::ecs::components::dynamic_tag, bounds_tag>> pack) {
		  for(auto [transform, vel] : pack) {
			  if(transform.position.x < bounds_min) {
				  vel.direction.x = std::abs(vel.direction.x);
			  } else if(transform.position.x > bounds_max) {
				  vel.direction.x = -std::abs(vel.direction.x);
			  }
			  if(transform.position.y < bounds_min) {
				  vel.direction.y = std::abs(vel.direction.y);
			  } else if(transform.position.y > bounds_max) {
				  vel.direction.y = -std::abs(vel.direction.y);
			  }
			  if(transform.position.z < bounds_min) {
				  vel.direction.z = std::abs(vel.direction.z);
			  } else if(transform.position.z > bounds_max) {
				  vel.direction.z = -std::abs(vel.direction.z);
			  }
		  }
	  });

	// create a bunch of spheres, 65^3 = 274625 to be exact
	// they will live for 14 to 34 seconds, and will be affected by an attractor in the middle of the
	// scene pulling them towards it.
	{
		auto instances = bundle->instantiate(geometryResource, range_i * range_i * range_i);
		psl::array<psl::vec3> colours {};
		colours.resize(instances.size());

		state.create(
		  range_i * range_i * range_i,
		  [&bundle, &geometryResource, id_it = instances.begin(), x = 0, y = 0, z = 0, colour_it = colours.begin()](
			psl::ecs::entity_t ent,
			core::ecs::components::renderable& renderable,
			core::ecs::components::transform& transform) mutable {
			  transform.position = psl::vec3 {float(x), float(y), float(z)};
			  transform.rotation = psl::quat::identity;
			  transform.scale	 = psl::vec3::one;
			  x += 1;
			  if(x >= range_i) {
				  x = 0;
				  y += 1;
			  }
			  if(y >= range_i) {
				  y = 0;
				  z += 1;
			  }

			  renderable.bundle		 = bundle;
			  renderable.geometry	 = geometryResource;
			  renderable.instance_id = *id_it;
			  ++id_it;


			  constexpr std::array<psl::vec3, 10> baseCols({{psl::vec3 {142, 202, 230} / 255.f},
															{psl::vec3 {33, 158, 188} / 255.f},
															{psl::vec3 {18, 103, 130} / 255.f},
															{psl::vec3 {2, 48, 71} / 255.f},
															{psl::vec3 {255, 183, 3} / 255.f},
															{psl::vec3 {253, 158, 2} / 255.f},
															{psl::vec3 {251, 133, 0} / 255.f},
															{psl::vec3 {187, 62, 3} / 255.f},
															{psl::vec3 {174, 32, 18} / 255.f},
															{psl::vec3 {155, 34, 38} / 255.f}});

			  auto xIndex	  = size_t(std::floor((transform.position[0] / 65.f) * baseCols.size()));
			  auto yIntensity = std::round((transform.position[1] / 65.f) * 7.f + 1.f) / 12.f;
			  auto zIntensity = std::round((transform.position[2] / 65.f) * 7.f + 1.f) / 12.f;
			  *colour_it =
				baseCols[xIndex % baseCols.size()] * (yIntensity + zIntensity + ((std::rand() % 100) / 400.f) + .25f);
			  ++colour_it;
		  },
		  psl::ecs::empty<core::ecs::components::transform_instance_data_tag>(),
		  psl::ecs::empty<core::ecs::components::dynamic_tag>(),
		  psl::ecs::empty<bounds_tag>(),
		  core::ecs::components::velocity(psl::vec3::up, 0.f, .3f),
		  [](core::ecs::components::lifetime& lifetime) { lifetime.value = 14.f + (std::rand() % 2000) / 100.f; });

		// we set this afterwards to avoid doing multiple allocations while creating the entities.
		bundle->set(geometryResource, instances, "INSTANCE_COLOR", colours);
	}

	float accumulated = 0.f;

	std::queue<float> frameTimes {};
	float totalTime		= 0.f;
	float const maxTime = 5.f;

	size_t tickCount = 0;

	state.create(
	  1,
	  core::ecs::components::transform {psl::vec3 {midpoint, midpoint, midpoint}, psl::vec3::one, psl::quat::identity},
	  core::ecs::components::lifetime {.1f},
	  core::ecs::components::attractor {1200.f, range_f});

	engine_instance.run([&state, &bundle, &geometryResource, &frameTimes, &totalTime, maxTime, &tickCount](
						  auto const& engine_instance,
						  std::chrono::duration<float> dTime,
						  std::chrono::duration<float> elapsed) {
		dTime = std::min(dTime, std::chrono::duration<float> {1.f});
		state.tick(dTime);
		core::log->info("Entities: {}, Instances: {}, triangles: {}",
						state.size(),
						bundle->instances(geometryResource.uid()),
						geometryResource->triangles() * bundle->instances(geometryResource.uid()));

		frameTimes.push(dTime.count());
		totalTime += dTime.count();
		while(totalTime > maxTime) {
			totalTime -= frameTimes.front();
			frameTimes.pop();
		}

		// every frame spawn 100 spheres with a lifetime of 2 seconds
		{
			const auto count = 100;
			auto ids		 = bundle->instantiate(geometryResource, count);
			psl::array<psl::vec3> colours {
			  psl::vec3 {1.0f, 0.5f, 0.5f},
			  psl::vec3 {0.5f, 0.5f, 1.0f},
			};
			colours.reserve(count);
			// fill colours to match count
			while(colours.size() < ids.size()) {
				colours.push_back(colours[colours.size() % 2]);
			}
			colours.resize(count);
			bundle->set(geometryResource, ids, "INSTANCE_COLOR", colours);
			state.create(
			  count,
			  [&bundle, &geometryResource, id_it = ids.begin()](core::ecs::components::renderable& renderable) mutable {
				  renderable.bundle		 = bundle;
				  renderable.geometry	 = geometryResource;
				  renderable.instance_id = *id_it;
				  ++id_it;
			  },
			  psl::ecs::empty<core::ecs::components::transform_instance_data_tag>(),
			  psl::ecs::empty<core::ecs::components::dynamic_tag>(),
			  core::ecs::components::transform {psl::vec3 {0, 0, -bounds_max}},
			  [](core::ecs::components::velocity& vel) {
				  vel.direction = psl::math::normalize(psl::vec3 {
					(std::rand() % 40 / 40.f) - .5f, std::rand() % 100 / 40.f, (std::rand() % 40 / 40.f) - .5f});
				  vel.force		= std::rand() % 1000 / 25.f + 10.f;
				  vel.inertia	= 1.f;
			  },
			  core::ecs::components::lifetime {2.f});
		}

		float avgTime = totalTime / float(frameTimes.size());
		core::log->info("Frame Time: {:.2f}ms, Avg: {:.2f}ms", dTime.count() * 1000.f, avgTime * 1000.f);
		++tickCount;
	});

	return 0;
}

int main(int argc, char** argv) {
	core::initialize_loggers(false, true);
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
