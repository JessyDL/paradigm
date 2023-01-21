#pragma once
#include "core/fwd/gfx/buffer.hpp"
#include "psl/ecs/state.hpp"
#include "resource/resource.hpp"

namespace psl::ecs {
class state_t;
}

namespace core::gfx {
class geometry_t;
class bundle;
class context;
class pipeline_cache;
}	 // namespace core::gfx

namespace core::ecs::components {
struct transform;
struct camera;
}	 // namespace core::ecs::components

namespace core::ecs::systems::debug {
/// \brief renders a grid-like object of given size.
class grid {
	struct tag {};

  public:
	grid(psl::ecs::state_t& state,
		 psl::ecs::entity_t target,
		 core::resource::cache_t& cache,
		 core::resource::handle<core::gfx::context> context,
		 core::resource::handle<core::gfx::buffer_t> vertexBuffer,
		 core::resource::handle<core::gfx::buffer_t> indexBuffer,
		 core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
		 core::resource::handle<core::gfx::shader_buffer_binding> instanceMaterialBuffer,
		 core::resource::handle<core::gfx::buffer_t> instanceVertexBuffer,
		 psl::vec3 scale  = psl::vec3::one * 64.0f,
		 psl::vec3 offset = psl::vec3::zero);
	~grid() = default;

	grid(const grid& other)				   = delete;
	grid(grid&& other) noexcept			   = delete;
	grid& operator=(const grid& other)	   = delete;
	grid& operator=(grid&& other) noexcept = delete;

  private:
	void tick(psl::ecs::info_t& info,
			  psl::ecs::pack_direct_full_t<psl::ecs::entity_t,
										   const core::ecs::components::transform,
										   psl::ecs::filter<core::ecs::components::camera>> pack,
			  psl::ecs::pack_direct_full_t<core::ecs::components::transform, psl::ecs::filter<grid::tag>> grid_pack);

	core::resource::handle<core::gfx::bundle> m_Bundle;
	core::resource::handle<core::gfx::geometry_t> m_Geometry;
	std::vector<psl::ecs::entity_t> m_Entities;
	psl::ecs::entity_t m_Target;
	psl::vec3 m_Scale;
	psl::vec3 m_Offset;
};

}	 // namespace core::ecs::systems::debug
