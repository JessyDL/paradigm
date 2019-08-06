#pragma once
#include "resource/resource.hpp"
#include "math/math.hpp"


namespace core::gfx
{
	class swapchain;
	class pass;
} // namespace core::gfx
namespace core::os
{
	class surface;
}
namespace core::ivk
{
	class buffer;
}
namespace core::ecs::components
{
	struct transform;
	struct renderable;
	struct camera;
} // namespace core::ecs::components

namespace psl::ecs
{
	class state;
	struct info;

	template <typename... Ts>
	class pack;
} // namespace psl::ecs
namespace core::ecs::systems
{
	class gpu_camera
	{

		const psl::mat4x4 clip{1.0f,  0.0f, 0.0f, 0.0f, +0.0f, -1.0f, 0.0f, 0.0f,
							   +0.0f, 0.0f, 0.5f, 0.0f, +0.0f, 0.0f,  0.5f, 1.0f};

	  public:
		struct framedata
		{
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
		};

		gpu_camera(psl::ecs::state& state, core::resource::handle<core::os::surface> surface,
				   core::resource::handle<core::ivk::buffer> buffer);
		void tick(psl::ecs::info& info,
				  psl::ecs::pack<const core::ecs::components::camera, const core::ecs::components::transform> cameras);

	  private:
		void update_buffer(size_t index, const core::ecs::components::transform& transform,
						   const core::ecs::components::camera& camera);

		core::resource::handle<core::os::surface> m_Surface;
		core::resource::handle<core::ivk::buffer> m_Buffer;
		std::vector<memory::segment> fdatasegment;
	};

} // namespace core::ecs::systems