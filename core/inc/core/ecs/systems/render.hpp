#pragma once
#include "core/gfx/drawgroup.hpp"
#include "core/resource/resource.hpp"
#include "psl/ecs/state.hpp"
#include "psl/math/matrix.hpp"
#include "psl/math/vec.hpp"
#include "psl/view_ptr.hpp"

namespace core::gfx {
class buffer_t;
class drawpass;
}	 // namespace core::gfx

namespace core::ecs::components {
struct transform;
struct renderable;
struct camera;
struct transform_instance_data_tag;
struct transform_instance_object_model_tag;
struct dynamic_tag;
}	 // namespace core::ecs::components

namespace core::ecs::systems {
class render {
	struct renderer_sort {
		bool operator()(const core::ecs::components::renderable& lhs,
						const core::ecs::components::renderable& rhs) const noexcept;
	};

  public:
	render(psl::ecs::state_t& state, psl::view_ptr<core::gfx::drawpass> pass);

	~render() = default;

	render(const render&)			 = delete;
	render(render&&)				 = delete;
	render& operator=(const render&) = delete;
	render& operator=(render&&)		 = delete;

	void add_render_range(uint32_t begin, uint32_t end);
	void remove_render_range(uint32_t begin, uint32_t end);

  private:
	void update_instance_data(
	  psl::ecs::pack_indirect_partial_t<
		const core::ecs::components::renderable,
		const core::ecs::components::transform,
		psl::ecs::filter<core::ecs::components::dynamic_tag, core::ecs::components::transform_instance_data_tag>/*,
		psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>*/> pack);

	void update_instance_object_model(
	  psl::ecs::pack_indirect_partial_t<const core::ecs::components::renderable,
										const core::ecs::components::transform,
										psl::ecs::filter<core::ecs::components::dynamic_tag,
														 core::ecs::components::transform_instance_object_model_tag>,
										psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>> pack);
	void release_renderable_instances(
	  psl::ecs::pack_indirect_partial_t<const core::ecs::components::renderable,
										psl::ecs::on_remove<core::ecs::components::renderable>,
										psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>> pack);
	void tick_draws(
	  psl::ecs::pack_indirect_full_t<const core::ecs::components::renderable,
									 psl::ecs::on_add<core::ecs::components::renderable>> renderables,
	  psl::ecs::pack_indirect_full_t<const core::ecs::components::renderable,
									 psl::ecs::on_remove<core::ecs::components::renderable>> broken_renderables);

	psl::view_ptr<core::gfx::drawpass> m_Pass {nullptr};

	core::gfx::drawgroup m_DrawGroup {};
	psl::array<std::pair<uint32_t, uint32_t>> m_RenderRanges {};
	std::mutex m_Mutex;
};
}	 // namespace core::ecs::systems
