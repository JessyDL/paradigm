#pragma once
#include "resource/resource.hpp"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "gfx/bundle.h"
#include "psl/bytell_map.h"
#include "psl/ecs/state.h"
#include "vk/geometry.h"

namespace core::ecs::systems
{

	class geometry_instancing
	{
		struct renderer_sort
		{
			inline bool operator()(const core::ecs::components::renderable& lhs,
				const core::ecs::components::renderable& rhs) const noexcept
			{
				if (lhs.bundle.uid() != rhs.bundle.uid())
					return lhs.bundle.uid() < rhs.bundle.uid();
				else
					return lhs.geometry.uid() < rhs.geometry.uid();
			}
		};
		struct geometry_instance
		{
			size_t startIndex;
			size_t count;
		};

		struct instance_id
		{
			uint32_t id;
		};
	public:
		geometry_instancing(psl::ecs::state& state);
		~geometry_instancing() = default;

		geometry_instancing(const geometry_instancing& other) = delete;
		geometry_instancing(geometry_instancing&& other) noexcept = delete;
		geometry_instancing& operator=(const geometry_instancing& other) = delete;
		geometry_instancing& operator=(geometry_instancing&& other) noexcept = delete;

	private:
		void dynamic_system(psl::ecs::info& info,
			psl::ecs::pack<const core::ecs::components::renderable, const core::ecs::components::transform, const core::ecs::components::dynamic_tag, psl::ecs::except<core::ecs::components::dont_render_tag>,
			psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>> geometry_pack);

		void static_add(psl::ecs::info& info,
			psl::ecs::pack<psl::ecs::entity, const core::ecs::components::renderable, const core::ecs::components::transform, psl::ecs::except<core::ecs::components::dynamic_tag>, psl::ecs::on_combine<const core::ecs::components::renderable, const core::ecs::components::transform>,
			psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>> geometry_pack);
		void static_remove(psl::ecs::info& info,
			psl::ecs::pack<psl::ecs::entity, const core::ecs::components::renderable, const instance_id, psl::ecs::except<core::ecs::components::dynamic_tag>, psl::ecs::on_break<const core::ecs::components::renderable, const core::ecs::components::transform>> geometry_pack);

		//void static_disable();
		//void static_enable(psl::ecs::info& info, psl::ecs::pack<psl::ecs::entity, const core::ecs::components::renderable, const instance_id, core::ecs::components::dont_render_tag> dont_render);
	};
} // namespace core::ecs::systems