#pragma once
#include "ecs/state.h"
#include "gfx/pass.h"
#include "systems/resource.h"
#include "gfx/drawgroup.h"
#include "math/vec.h"
#include "math/matrix.h"

namespace core::gfx
{
	class context;
	class swapchain;
	class buffer;
} // namespace core::gfx
namespace core::os
{
	class surface;
}
namespace core::ecs::components
{
	struct transform;
	struct renderable;
	struct camera;

	struct render_layer
	{
		psl::string8_t name;
		uint32_t priority;
	};
} // namespace core::ecs::components

namespace core::ecs::systems
{
	class render
	{
	  public:
		  render(psl::ecs::state& state, core::resource::handle<core::gfx::context> context,
			  core::resource::handle<core::gfx::swapchain> swapchain);
		  render(psl::ecs::state& state, core::resource::handle<core::gfx::context> context,
			  core::resource::handle<core::gfx::framebuffer> framebuffer);

		~render() = default;

		render(const render&) = delete;
		render(render&&)	  = delete;
		render& operator=(const render&) = delete;
		render& operator=(render&&) = delete;

		core::gfx::pass& pass() noexcept { return m_Pass; ; }
	  private:
		void tick_draws(psl::ecs::info& info,
			psl::ecs::pack<const core::ecs::components::transform, const core::ecs::components::renderable,
							psl::ecs::on_combine<core::ecs::components::transform, core::ecs::components::renderable>>
				renderables,
			psl::ecs::pack<const core::ecs::components::transform, const core::ecs::components::renderable,
							psl::ecs::on_break<core::ecs::components::transform, core::ecs::components::renderable>>
				broken_renderables);

		core::gfx::pass m_Pass;

		core::gfx::drawgroup m_DrawGroup{};
	};
} // namespace core::ecs::systems