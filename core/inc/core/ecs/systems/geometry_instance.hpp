#pragma once
#include "core/ecs/components/renderable.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/gfx/bundle.hpp"
#include "core/resource/resource.hpp"
#include "psl/ecs/order_by.hpp"
#include "psl/ecs/state.hpp"

namespace core::ecs::systems {
class geometry_instancing {
	struct renderer_sort {
		inline bool operator()(const core::ecs::components::renderable& lhs,
							   const core::ecs::components::renderable& rhs) const noexcept {
			if(lhs.bundle.uid() != rhs.bundle.uid())
				return lhs.bundle.uid() < rhs.bundle.uid();
			else
				return lhs.geometry.uid() < rhs.geometry.uid();
		}
	};
	struct geometry_instance {
		size_t startIndex;
		size_t count;
	};

	struct instance_id {
		uint32_t id;
	};

	struct instance_id_sort {
		inline bool operator()(const instance_id& lhs, const instance_id& rhs) const noexcept {
			return lhs.id < rhs.id;
		}
	};

  public:
	geometry_instancing(psl::ecs::state_t& state);
	~geometry_instancing() = default;

	geometry_instancing(const geometry_instancing& other)				 = delete;
	geometry_instancing(geometry_instancing&& other) noexcept			 = delete;
	geometry_instancing& operator=(const geometry_instancing& other)	 = delete;
	geometry_instancing& operator=(geometry_instancing&& other) noexcept = delete;

  private:
	/* void dynamic_add(psl::ecs::info& info,
					 psl::ecs::pack_direct_full_t<core::ecs::components::renderable,
									psl::ecs::filter<const core::ecs::components::dynamic_tag>,
									psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>> pack);
	*/
	void dynamic_add(
	  psl::ecs::info_t& info,
	  psl::ecs::pack_indirect_full_t<
		psl::ecs::entity_t,
		core::ecs::components::renderable,
		const core::ecs::components::transform,
		psl::ecs::filter<core::ecs::components::dynamic_tag>,
		psl::ecs::except<core::ecs::components::dont_render_tag>,
		psl::ecs::on_combine<core::ecs::components::renderable, core::ecs::components::transform>> geometry_pack);

	void dynamic_remove(
	  psl::ecs::info_t& info,
	  psl::ecs::pack_direct_partial_t<
		psl::ecs::entity_t,
		core::ecs::components::renderable,
		const instance_id,
		psl::ecs::filter<core::ecs::components::dynamic_tag>,
		psl::ecs::except<core::ecs::components::dont_render_tag>,
		psl::ecs::on_break<core::ecs::components::renderable, core::ecs::components::transform, instance_id>> pack);
	void dynamic_update(psl::ecs::info_t& info,
						psl::ecs::pack_indirect_partial_t<core::ecs::components::renderable,
														  const core::ecs::components::transform,
														  const instance_id,
														  psl::ecs::filter<core::ecs::components::dynamic_tag>,
														  psl::ecs::except<core::ecs::components::dont_render_tag>,
														  psl::ecs::order_by<instance_id_sort, instance_id>> pack);
	void dynamic_system(
	  psl::ecs::info_t& info,
	  psl::ecs::pack_direct_full_t<core::ecs::components::renderable,
								   const core::ecs::components::transform,
								   const core::ecs::components::dynamic_tag,
								   psl::ecs::except<core::ecs::components::dont_render_tag>,
								   psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>> geometry_pack);

	void
	static_add(psl::ecs::info_t& info,
			   psl::ecs::pack_direct_full_t<
				 psl::ecs::entity_t,
				 const core::ecs::components::renderable,
				 const core::ecs::components::transform,
				 psl::ecs::except<core::ecs::components::dynamic_tag>,
				 psl::ecs::on_combine<const core::ecs::components::renderable, const core::ecs::components::transform>,
				 psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>> geometry_pack);
	void static_remove(
	  psl::ecs::info_t& info,
	  psl::ecs::pack_direct_full_t<psl::ecs::entity_t,
								   core::ecs::components::renderable,
								   const instance_id,
								   psl::ecs::except<core::ecs::components::dynamic_tag>,
								   psl::ecs::on_break<const core::ecs::components::renderable,
													  const core::ecs::components::transform>> geometry_pack);

	void static_geometry_add(psl::ecs::info_t& info,
							 psl::ecs::pack_direct_full_t<psl::ecs::entity_t,
														  const core::ecs::components::renderable,
														  psl::ecs::except<core::ecs::components::transform>,
														  psl::ecs::on_add<core::ecs::components::renderable>> pack);


	void
	static_geometry_remove(psl::ecs::info_t& info,
						   psl::ecs::pack_direct_full_t<psl::ecs::entity_t,
														core::ecs::components::renderable,
														const instance_id,
														psl::ecs::except<core::ecs::components::transform>,
														psl::ecs::on_remove<core::ecs::components::renderable>> pack);
	std::mutex m_Mutex;
	// void static_disable();
	// void static_enable(psl::ecs::info& info, psl::ecs::pack_direct_full_t<psl::ecs::entity_t, const
	// core::ecs::components::renderable, const instance_id, core::ecs::components::dont_render_tag> dont_render);
};
}	 // namespace core::ecs::systems
