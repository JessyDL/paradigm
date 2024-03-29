#pragma once
#include "core/ecs/components/lighting.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/ecs/systems/render.hpp"
#include "core/resource/resource.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/ecs/selectors.hpp"
#include "psl/view_ptr.hpp"
#include <stdint.h>

namespace core::gfx {
class context;
class render_graph;
class drawpass;
class buffer_t;
}	 // namespace core::gfx

namespace core::os {
class surface;
}

namespace psl::ecs {
class state_t;
struct info_t;
template <typename... Ts>
class pack;
}	 // namespace psl::ecs

namespace memory {
class region;
}


namespace core::ecs::systems {
//	class render;

class lighting_system {
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
	  psl::ecs::pack_direct_full_t<psl::ecs::entity_t,
								   core::ecs::components::light,
								   psl::ecs::on_combine<core::ecs::components::light, core::ecs::components::transform>>
		pack);

  private:
	psl::view_ptr<core::resource::cache_t> m_Cache;
	psl::view_ptr<core::gfx::render_graph> m_RenderGraph;
	psl::view_ptr<core::gfx::drawpass> m_DependsPass;
	psl::view_ptr<psl::ecs::state_t> m_State;
	core::resource::handle<core::gfx::context> m_Context;
	core::resource::handle<core::os::surface> m_Surface;
	core::resource::handle<core::gfx::buffer_t> m_LightDataBuffer;
	memory::segment m_LightSegment;

	// std::unordered_map<psl::ecs::entity_t, psl::view_ptr<core::gfx::drawpass>> m_Passes;
	std::unordered_map<psl::ecs::entity_t::size_type, psl::unique_ptr<core::ecs::systems::render>> m_Systems;
};

}	 // namespace core::ecs::systems
