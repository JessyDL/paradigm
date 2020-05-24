#include "ecs/systems/render.h"
#include "ecs/components/transform.h"
#include "ecs/components/renderable.h"
#include "gfx/geometry.h"
#include "gfx/bundle.h"
#include "gfx/drawpass.h"
#include "gfx/render_graph.h"

using core::resource::handle;
using namespace core::gfx;
using namespace core;
using namespace psl::ecs;
using namespace core::ecs::systems;
using namespace core::ecs::components;
using namespace psl;

using std::chrono::duration;

render::render(state& state, psl::view_ptr<core::gfx::drawpass> pass) : m_Pass(pass)
{
	state.declare(threading::seq, &render::tick_draws, this);
}
void render::tick_draws(info& info,
						pack<const renderable, on_add<renderable>> renderables,
						pack<const renderable, on_remove<renderable>> broken_renderables)
{
	if(!renderables.size() && !broken_renderables.size()) return;
	m_Pass->dirty(true);
	m_Pass->clear();

	// for each RenderRange, create a drawgroup. assign all bundles to that group
	for(auto renderRange : m_RenderRanges)
	{
		auto& default_layer = m_DrawGroup.layer("default", renderRange.first, renderRange.second - renderRange.first);
		for(auto [renderable] : renderables)
		{
			if(renderable.bundle)
				m_DrawGroup.add(default_layer, renderable.bundle.make_shared()).add(renderable.geometry.make_shared());
		}

		for(auto [renderable] : broken_renderables)
		{
			if(!renderable.bundle) continue;
			if(auto dCall = m_DrawGroup.get(default_layer, renderable.bundle.make_shared()))
			{
				dCall.value().get().remove(renderable.geometry.operator const psl::UID&());
			}
		}
	}
	m_Pass->add(m_DrawGroup);
}

void render::add_render_range(uint32_t begin, uint32_t end) { m_RenderRanges.emplace_back(std::make_pair(begin, end)); }

void render::remove_render_range(uint32_t begin, uint32_t end) { throw std::runtime_error("not implemented"); }