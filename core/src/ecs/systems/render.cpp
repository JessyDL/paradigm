#include "ecs/systems/render.h"
#include "ecs/components/transform.h"
#include "ecs/components/renderable.h"
#include "ecs/components/camera.h"
#include "gfx/material.h"
#include "data/material.h"
#include "vk/geometry.h"

#include "vk/buffer.h"

using core::resource::handle;
using namespace core::gfx;
using namespace core::os;
using namespace core;
using namespace psl::ecs;
using namespace core::ecs::systems;
using namespace core::ecs::components;
using namespace psl;

using std::chrono::duration;

render::render(state& state, core::gfx::pass& pass) : m_Pass(pass)
{
	state.declare(threading::seq, &render::tick_draws, this);
}

void render::tick_draws(info& info,
						pack<const transform, const renderable, on_combine<transform, renderable>> renderables,
						pack<const transform, const renderable, on_break<transform, renderable>> broken_renderables)
{
	{
		m_Pass.clear();
		auto& default_layer = m_DrawGroup.layer("default", 0);
		auto res			= renderables.get<const transform>();
		for(auto [transform, renderable] : renderables)
		{
			if(!std::any_of(std::begin(m_RenderRanges), std::end(m_RenderRanges),
							[priority = renderable.material.handle()->data()->render_layer()](
								const std::pair<uint32_t, uint32_t>& range) {
								return range.first <= priority && priority <= range.second;
							}))
				continue;
			m_DrawGroup.add(default_layer, renderable.material).add(renderable.geometry);
		}

		for(auto [transform, renderable] : broken_renderables)
		{
			if(auto dCall = m_DrawGroup.get(default_layer, renderable.material))
			{
				if(!std::any_of(std::begin(m_RenderRanges), std::end(m_RenderRanges),
								[priority = renderable.material.handle()->data()->render_layer()](
									const std::pair<uint32_t, uint32_t>& range) {
									return range.first <= priority && priority <= range.second;
								}))
					continue;
				dCall.value().get().remove(renderable.geometry.operator const psl::UID&());
			}
		}

		m_Pass.add(m_DrawGroup);
	}
}

void render::add_render_range(uint32_t begin, uint32_t end) {
	m_RenderRanges.emplace_back(std::make_pair(begin, end));
}

void render::remove_render_range(uint32_t begin, uint32_t end) {
	throw std::runtime_error("not implemented");
}