#pragma once
#include "view_ptr.h"
#include "ecs/selectors.h"
#include "ecs/entity.h"
#include "ecs/components/transform.h"
#include "ecs/components/lighting.h"
#include "resource/resource.hpp"
#include "ecs/systems/render.h"

namespace core::ivk
{
	class context;
}
namespace core::gfx
{
	class render_graph;
	class pass;
	class buffer;
} // namespace core::gfx

namespace core::os
{
	class surface;
}

namespace psl::ecs
{
	class state;
	struct info;
	template <typename... Ts>
	class pack;
} // namespace psl::ecs

namespace memory
{
	class region;
}


namespace core::ecs::systems
{
	//	class render;

	class lighting_system
	{
	  public:
		lighting_system(psl::view_ptr<psl::ecs::state> state, psl::view_ptr<core::resource::cache> cache,
						memory::region& resource_region, psl::view_ptr<core::gfx::render_graph> renderGraph,
						psl::view_ptr<core::gfx::pass> pass, core::resource::handle<core::ivk::context> context,
						core::resource::handle<core::os::surface> surface) noexcept;

		~lighting_system() = default;

		void
		create_dir(psl::ecs::info& info,
				   psl::ecs::pack<psl::ecs::entity, core::ecs::components::light,
								  psl::ecs::on_combine<core::ecs::components::light, core::ecs::components::transform>>
					   pack);

	  private:
		psl::view_ptr<core::resource::cache> m_Cache;
		psl::view_ptr<core::gfx::render_graph> m_RenderGraph;
		psl::view_ptr<core::gfx::pass> m_DependsPass;
		psl::view_ptr<psl::ecs::state> m_State;
		core::resource::handle<core::ivk::context> m_Context;
		core::resource::handle<core::os::surface> m_Surface;
		core::resource::handle<core::ivk::buffer> m_LightDataBuffer;
		memory::segment m_LightSegment;

		std::unordered_map<psl::ecs::entity, psl::view_ptr<core::gfx::pass>> m_Passes;
		std::unordered_map<psl::ecs::entity, psl::unique_ptr<core::ecs::systems::render>> m_Systems;
	};

} // namespace core::ecs::systems