#include "ecs/systems/render.h"
#include "ecs/components/transform.h"
#include "ecs/components/renderable.h"
#include "ecs/components/camera.h"
#include "vk/swapchain.h"
#include "vk/framebuffer.h"
#include "vk/context.h"
#include "os/surface.h"
#include "gfx/material.h"
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

render::render(state& state, handle<context> context, handle<swapchain> swapchain)
	: m_Pass(context, swapchain)
{
	state.declare(threading::seq, &render::tick_draws, this);
}


render::render(state& state, handle<context> context, handle<framebuffer> framebuffer)
	: m_Pass(context, framebuffer)
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
			m_DrawGroup.add(default_layer, renderable.material).add(renderable.geometry);
		}

		for(auto [transform, renderable] : broken_renderables)
		{
			if(auto dCall = m_DrawGroup.get(default_layer, renderable.material))
			{
				dCall.value().get().remove(renderable.geometry.operator const psl::UID&());
			}
		}

		m_Pass.add(m_DrawGroup);
		m_Pass.build();
	}
	m_Pass.prepare();
	//m_Pass.present();
}