#pragma once
#include "ecs/components/text.hpp"
#include "fwd/gfx/buffer.hpp"
#include "fwd/gfx/bundle.hpp"
#include "fwd/gfx/context.hpp"
#include "fwd/gfx/geometry.hpp"
#include "fwd/gfx/pipeline_cache.hpp"
#include "fwd/gfx/texture.hpp"
#include "psl/array.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/ecs/pack.hpp"
#include "psl/ecs/selectors.hpp"
#include "psl/math/vec.hpp"
#include "resource/handle.hpp"

namespace core::data {
class geometry_t;
}
namespace psl::ecs {
class state_t;
struct info_t;
}	 // namespace psl::ecs

namespace core::ecs::components {
struct renderable;
struct dynamic_tag;
}	 // namespace core::ecs::components
namespace core::ecs::systems {
class text {
	struct character_t {
		psl::vec4 uv;
		psl::vec4 quad;
		psl::vec2 offset;
	};

  public:
	text(psl::ecs::state_t& state,
		 core::resource::cache_t& cache,
		 core::resource::handle<core::gfx::context> context,
		 core::resource::handle<core::gfx::buffer_t> vertexBuffer,
		 core::resource::handle<core::gfx::buffer_t> indexBuffer,
		 core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
		 core::resource::handle<core::gfx::buffer_t> materialBuffer,
		 core::resource::handle<core::gfx::buffer_t> vertexInstanceBuffer,
		 core::resource::handle<core::gfx::shader_buffer_binding> materialInstanceBuffer);
	~text() = default;

	text(const text& other)				   = delete;
	text(text&& other) noexcept			   = delete;
	text& operator=(const text& other)	   = delete;
	text& operator=(text&& other) noexcept = delete;


  private:
	void update_dynamic(psl::ecs::info_t& info,
						psl::ecs::pack_direct_partial_t<psl::ecs::entity_t,
														core::ecs::components::text,
														core::ecs::components::renderable,
														psl::ecs::filter<core::ecs::components::dynamic_tag>> pack);
	void add(psl::ecs::info_t& info,
			 psl::ecs::pack_direct_partial_t<psl::ecs::entity_t,
											 core::ecs::components::text,
											 psl::ecs::on_add<core::ecs::components::text>> pack);
	void remove(psl::ecs::info_t& info,
				psl::ecs::pack_direct_partial_t<psl::ecs::entity_t,
												core::ecs::components::text,
												psl::ecs::on_remove<core::ecs::components::text>> pack);

	core::resource::handle<core::data::geometry_t> create_text(psl::string_view text);

	psl::array<character_t> character_data;
	core::resource::handle<core::gfx::texture_t> m_FontTexture;
	core::resource::handle<core::gfx::context> m_Context;
	core::resource::handle<core::gfx::buffer_t> m_VertexBuffer;
	core::resource::handle<core::gfx::buffer_t> m_IndexBuffer;
	core::resource::handle<core::gfx::bundle> m_Bundle;
	core::resource::cache_t& m_Cache;
};
}	 // namespace core::ecs::systems
