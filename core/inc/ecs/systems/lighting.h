#pragma once
#include "ecs/components/lighting.h"
#include "ecs/components/transform.h"
#include "ecs/systems/render.h"
#include "psl/ecs/entity.h"
#include "psl/ecs/selectors.h"
#include "psl/view_ptr.h"
#include "resource/resource.hpp"

namespace core::gfx
{
	class context;
	class render_graph;
	class drawpass;
	class buffer_t;
}	 // namespace core::gfx

namespace core::os
{
	class surface;
}

namespace psl::ecs
{
	class state_t;
	struct info_t;
	template <typename... Ts>
	class pack;
}	 // namespace psl::ecs

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
		lighting_system(psl::view_ptr<psl::ecs::state_t> state,
						psl::view_ptr<core::resource::cache_t> cache,
						memory::region& resource_region,
						psl::view_ptr<core::gfx::render_graph> renderGraph,
						psl::view_ptr<core::gfx::drawpass> pass,
						core::resource::handle<core::gfx::context> context,
						core::resource::handle<core::os::surface> surface) noexcept;

		~lighting_system() = default;

		void create_dir(
		  psl::ecs::info_t& info,
		  psl::ecs::pack<psl::ecs::entity,
						 core::ecs::components::light,
						 psl::ecs::on_combine<core::ecs::components::light, core::ecs::components::transform>> pack);

	  private:
		psl::view_ptr<core::resource::cache_t> m_Cache;
		psl::view_ptr<core::gfx::render_graph> m_RenderGraph;
		psl::view_ptr<core::gfx::drawpass> m_DependsPass;
		psl::view_ptr<psl::ecs::state_t> m_State;
		core::resource::handle<core::gfx::context> m_Context;
		core::resource::handle<core::os::surface> m_Surface;
		core::resource::handle<core::gfx::buffer_t> m_LightDataBuffer;
		memory::segment m_LightSegment;

		// std::unordered_map<psl::ecs::entity, psl::view_ptr<core::gfx::drawpass>> m_Passes;
		std::unordered_map<psl::ecs::entity, psl::unique_ptr<core::ecs::systems::render>> m_Systems;
	};

}	 // namespace core::ecs::systems