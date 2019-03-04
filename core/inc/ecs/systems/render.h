#pragma once
#include "ecs/state.h"
#include "gfx/pass.h"
#include "systems/resource.h"
#include "gfx/drawgroup.h"
#include "math/vec.h"
#include "math/matrix.h"

namespace core::gfx
{
	class buffer;
} // namespace core::gfx

namespace core::ecs::components
{
	struct transform;
	struct renderable;
	struct camera;
} // namespace core::ecs::components

namespace core::ecs::systems
{
	class render
	{
	  public:
		  render(psl::ecs::state& state, core::gfx::pass& pass);

		~render() = default;

		render(const render&) = delete;
		render(render&&)	  = delete;
		render& operator=(const render&) = delete;
		render& operator=(render&&) = delete;

		void add_render_range(int begin, int end);
		void remove_render_range(int begin, int end);
	  private:
		void tick_draws(psl::ecs::info& info,
			psl::ecs::pack<const core::ecs::components::transform, const core::ecs::components::renderable,
							psl::ecs::on_combine<core::ecs::components::transform, core::ecs::components::renderable>>
				renderables,
			psl::ecs::pack<const core::ecs::components::transform, const core::ecs::components::renderable,
							psl::ecs::on_break<core::ecs::components::transform, core::ecs::components::renderable>>
				broken_renderables);

		core::gfx::pass& m_Pass;

		core::gfx::drawgroup m_DrawGroup{};
		psl::array<std::pair<int, int>> m_RenderRanges;
	};
} // namespace core::ecs::systems