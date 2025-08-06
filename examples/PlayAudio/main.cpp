

// This example demonstrates creating a simple audio object, and mutating it (todo).


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "core/data/geometry.hpp"
#include "core/data/material.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/renderable.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/ecs/systems/geometry_instance.hpp"
#include "core/ecs/systems/gpu_camera.hpp"
#include "core/ecs/systems/render.hpp"
#include "core/gfx/geometry.hpp"
#include "core/gfx/material.hpp"
#include "core/gfx/pipeline_cache.hpp"
#include "core/gfx/types.hpp"
#include "core/logging.hpp"
#include "core/os/context.hpp"
#include "core/resource/resource.hpp"

#include "core/audio/audio.hpp"
#include "core/audio/engine.hpp"
#include "core/meta/audio.hpp"

#include "core/ecs/components/audio.hpp"
#include "core/ecs/systems/audio.hpp"

#include "examples/cli_parser.hpp"
#include "examples/engine_instance.hpp"

int entry(core::gfx::graphics_backend backend, std::unique_ptr<core::os::context> os_context) {
	engine_instance_t::options_t options {};
	options.backend			 = backend;
	options.application_name = "PlayAudio Example";
	engine_instance_t engine_instance {options, std::move(os_context)};


	auto audioEngineHandle = engine_instance.cache().create<core::audio::engine_t>();
	engine_instance.cache().library().set(audioEngineHandle.uid(), "GLOBAL_AUDIO_ENGINE");

	auto audioHandle = engine_instance.cache().instantiate<core::audio::audio_t>(
	  "24469495-a1d4-ba64-77ed-f22be221f498"_uid, audioEngineHandle);
	psl_assert(audioHandle, "Audio resource was missing");

	psl::ecs::state_t state {};
	auto gpuCameraSystem = core::ecs::systems::gpu_camera {
	  state, engine_instance.surface(), engine_instance.frame_cam_buffer_binding(), backend};
	auto geometryInstancingSystem = core::ecs::systems::geometry_instancing {state};
	auto renderSystem			  = core::ecs::systems::render {state, engine_instance.swapchain()};
	renderSystem.add_render_range(0, 1000);

	auto audioSystem = core::ecs::systems::audio_t(state, audioEngineHandle);

	state.create(
	  1, core::ecs::components::transform {psl::vec3 {0, 0, -2}}, psl::ecs::empty<core::ecs::components::camera> {});

	auto audioEntity = state.create(
	  1,
	  core::ecs::components::audio_resource_t {audioHandle},
	  core::ecs::components::audio_settings_t {.volume = 1.0f, .pitch = 1.0f, .loop = false, .paused = false});


	engine_instance.run([&state, audioEntity, audioHandle](auto const& engine_instance,
														   std::chrono::duration<float> dTime,
														   std::chrono::duration<float> elapsed) {
		state.tick(dTime);
		// todo(jdl): when the UI is done add controls so you can control the audio settings.
		/*state.mutate_components<core::ecs::components::audio_settings_t>(
		  audioEntity,
		  core::ecs::components::audio_settings_t {
			.volume = std::sin(elapsed.count() * 2.f - 1.f), .pitch = 1.0f, .loop = false, .paused = false});*/
	});

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
