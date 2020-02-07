#pragma once
#include "psl/ecs/entity.h"
#include "psl/ecs/selectors.h"
#include "ecs/components/text.h"
#include "resource/handle.h"
#include "fwd/gfx/texture.h"
#include "fwd/gfx/context.h"
#include "fwd/gfx/buffer.h"
#include "fwd/gfx/geometry.h"
#include "fwd/gfx/pipeline_cache.h"
#include "fwd/gfx/bundle.h"
#include "psl/array.h"
#include "psl/math/vec.h"

namespace core::data
{
	class geometry;
}
namespace psl::ecs
{
	class state;
	struct info;

	template <typename... Ts>
	class pack;
} // namespace psl::ecs

namespace core::ecs::components
{
	struct renderable;
	struct dynamic_tag;
}
namespace core::ecs::systems
{
	class text
	{
		struct character_t
		{
			psl::vec4 uv;
			psl::vec4 quad;
			psl::vec2 offset;
		};

	  public:
		text(psl::ecs::state& state, core::resource::cache& cache, core::resource::handle<core::gfx::context> context,
			 core::resource::handle<core::gfx::buffer> vertexBuffer,
			 core::resource::handle<core::gfx::buffer> indexBuffer,
			 core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
			 core::resource::handle<core::gfx::buffer> materialBuffer,
			 core::resource::handle<core::gfx::buffer> instanceBuffer);
		~text() = default;

		text(const text& other)		= delete;
		text(text&& other) noexcept = delete;
		text& operator=(const text& other) = delete;
		text& operator=(text&& other) noexcept = delete;


	  private:
		void update_dynamic(
			psl::ecs::info& info,
			psl::ecs::pack<psl::ecs::partial, psl::ecs::entity, core::ecs::components::text,
						   core::ecs::components::renderable, psl::ecs::filter<core::ecs::components::dynamic_tag>>
				pack);
		void add(psl::ecs::info& info, psl::ecs::pack<psl::ecs::partial, psl::ecs::entity, core::ecs::components::text,
													  psl::ecs::on_add<core::ecs::components::text>>
										   pack);
		void remove(psl::ecs::info& info,
					psl::ecs::pack<psl::ecs::partial, psl::ecs::entity, core::ecs::components::text,
								   psl::ecs::on_remove<core::ecs::components::text>>
						pack);

		core::resource::handle<core::data::geometry> create_text(psl::string_view text);

		psl::array<character_t> character_data;
		core::resource::handle<core::gfx::texture> m_FontTexture;
		core::resource::handle<core::gfx::context> m_Context;
		core::resource::handle<core::gfx::buffer> m_VertexBuffer;
		core::resource::handle<core::gfx::buffer> m_IndexBuffer;
		core::resource::handle<core::gfx::bundle> m_Bundle;
		core::resource::cache& m_Cache;
	};
} // namespace core::ecs::systems