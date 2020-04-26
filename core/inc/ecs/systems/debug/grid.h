#pragma once
#include "psl/ecs/state.h"
#include "resource/resource.hpp"
#include "fwd/gfx/buffer.h"

namespace psl::ecs
{
	class state;
}

namespace core::gfx
{
	class geometry;
	class bundle;
	class context;
	class pipeline_cache;
} // namespace core::gfx

namespace core::ecs::components
{
	struct transform;
	struct camera;
} // namespace core::ecs::components

namespace core::ecs::systems::debug
{
	/// \brief renders a grid-like object of given size.
	class grid
	{
		struct tag
		{};
	  public:
		grid(psl::ecs::state& state, psl::ecs::entity target, core::resource::cache& cache,
			 core::resource::handle<core::gfx::context> context, core::resource::handle<core::gfx::buffer> vertexBuffer,
			 core::resource::handle<core::gfx::buffer> indexBuffer,
			 core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
			 core::resource::handle<core::gfx::shader_buffer_binding> instanceMaterialBuffer,
			core::resource::handle<core::gfx::buffer> instanceVertexBuffer, psl::vec3 scale = psl::vec3::one * 64.0f, psl::vec3 offset = psl::vec3::zero);
		~grid() = default;

		grid(const grid& other)		= delete;
		grid(grid&& other) noexcept = delete;
		grid& operator=(const grid& other) = delete;
		grid& operator=(grid&& other) noexcept = delete;

	  private:
		void tick(psl::ecs::info& info,
				  psl::ecs::pack<psl::ecs::entity, const core::ecs::components::transform,
								 psl::ecs::filter<core::ecs::components::camera>>
					  pack, psl::ecs::pack<core::ecs::components::transform, psl::ecs::filter<grid::tag>> grid_pack);

		core::resource::handle<core::gfx::bundle> m_Bundle;
		core::resource::handle<core::gfx::geometry> m_Geometry;
		std::vector<psl::ecs::entity> m_Entities;
		psl::ecs::entity m_Target;
		psl::vec3 m_Scale;
		psl::vec3 m_Offset;
	};

} // namespace core::ecs::systems::debug