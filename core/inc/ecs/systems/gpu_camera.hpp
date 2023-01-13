#pragma once
#include "gfx/context.hpp"
#include "psl/math/math.hpp"
#include "psl/memory/segment.hpp"
#include "resource/resource.hpp"
#include "psl/ecs/pack.hpp"

namespace core::gfx {
class buffer_t;
struct shader_buffer_binding;
}	 // namespace core::gfx
namespace core::os {
class surface;
}

namespace core::ecs::components {
struct transform;
struct renderable;
struct camera;
}	 // namespace core::ecs::components

namespace psl::ecs {
class state_t;
struct info_t;
}	 // namespace psl::ecs

namespace core::ecs::systems {
class gpu_camera {
	const psl::mat4x4
	  clip {1.0f, 0.0f, 0.0f, 0.0f, +0.0f, -1.0f, 0.0f, 0.0f, +0.0f, 0.0f, 0.5f, 0.0f, +0.0f, 0.0f, 0.5f, 1.0f};

  public:
	struct framedata {
		psl::mat4x4 clipMatrix;
		psl::mat4x4 projectionMatrix;
		psl::mat4x4 modelMatrix;
		psl::mat4x4 viewMatrix;
		psl::mat4x4 WVP;
		psl::mat4x4 VP;
		psl::vec4 ScreenParams;
		psl::vec4 GameTimer;
		psl::vec4 Parameters;
		psl::vec4 FogColor;
		psl::vec4 viewPos;
		psl::vec4 viewDir;
		psl::quat viewDirQuat;
	};

	gpu_camera(psl::ecs::state_t& state,
			   core::resource::handle<core::os::surface> surface,
			   core::resource::handle<core::gfx::shader_buffer_binding> binding,
			   core::gfx::graphics_backend backend);
	void tick(psl::ecs::info_t& info,
			  psl::ecs::pack_direct_full_t<const core::ecs::components::camera, const core::ecs::components::transform>
				cameras);

  private:
	void update_buffer(size_t index,
					   const core::ecs::components::transform& transform,
					   const core::ecs::components::camera& camera);

	core::resource::handle<core::os::surface> m_Surface;
	core::resource::handle<core::gfx::shader_buffer_binding> m_Binding;
	size_t m_Max {0};
	core::gfx::graphics_backend m_Backend;
};

}	 // namespace core::ecs::systems
