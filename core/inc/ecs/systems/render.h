#pragma once
#include "ecs/ecs.hpp"
#include "gfx/drawgroup.h"
#include "gfx/pass.h"
#include "systems/resource.h"
#include "math/math.hpp"

// todo split camera into its own component
#include "ecs/components/transform.h"
#include "ecs/components/camera.h"

namespace core::gfx
{
	class context;
	class swapchain;
	class buffer;
}
namespace core::os
{
	class surface;
}
namespace core::ecs::components
{
	struct transform;
	struct renderable;
	struct camera;
}
namespace core::ecs::systems
{
	class render
	{
		const glm::mat4 clip{1.0f,  0.0f, 0.0f, 0.0f, +0.0f, -1.0f, 0.0f, 0.0f,
			+0.0f, 0.0f, 0.5f, 0.0f, +0.0f, 0.0f,  0.5f, 1.0f};
			   
		core::ecs::vector<core::ecs::components::transform, core::ecs::READ_ONLY> m_Transforms;
		core::ecs::vector<core::ecs::components::renderable, core::ecs::READ_ONLY> m_Renderers;
		core::ecs::vector<core::ecs::entity> m_RenderableEntities;

		core::ecs::vector<core::ecs::components::camera, core::ecs::READ_ONLY> m_Cameras;
		core::ecs::vector<core::ecs::components::transform, core::ecs::READ_ONLY> m_CameraTransforms;
		core::ecs::vector<core::ecs::entity> m_CameraEntities;
	public:
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

		render(core::resource::handle<core::gfx::context> context, 
			core::resource::handle<core::gfx::swapchain> swapchain, 
			core::resource::handle<core::os::surface> surface,
			core::resource::handle<core::gfx::buffer> buffer);
		void announce(core::ecs::state& state);
		void tick(core::ecs::state& state, std::chrono::duration<float> dTime);

	private:
		void update_buffer(size_t index, const core::ecs::components::transform& transform, const core::ecs::components::camera& camera);

		core::gfx::drawgroup m_DrawGroup{};
		core::gfx::pass m_Pass;
		core::resource::handle<core::gfx::swapchain> m_Swapchain;
		core::resource::handle<core::os::surface> m_Surface;

		std::vector<memory::segment> fdatasegment;
		core::resource::handle<core::gfx::buffer> m_Buffer;
		framedata fdata{};
	};
}