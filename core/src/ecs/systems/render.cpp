
#include "ecs/systems/render.h"
#include "ecs/components/transform.h"
#include "ecs/components/renderable.h"
#include "ecs/components/camera.h"
#include "vk/swapchain.h"
#include "vk/context.h"
#include "os/surface.h"
#include "gfx/material.h"
#include "vk/geometry.h"

#include "vk/buffer.h"

using namespace core::resource;
using namespace core::gfx;
using namespace core::os;
using namespace core;
using namespace core::ecs::systems;
using namespace core::ecs::components;
using namespace psl;

render::render(core::ecs::state& state, 
			   handle<context> context, 
			   handle<swapchain> swapchain, 
			   handle<surface> surface,
			   handle<buffer> buffer)
	: m_Pass(context, swapchain), m_Swapchain(swapchain), m_Surface(surface), m_Buffer(buffer)
{
	state.declare(threading::seq, &render::tick_cameras, this);
	state.declare(threading::seq, &render::tick_draws, this);
}

core::ecs::command_buffer render::tick_cameras(
	const core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
	core::ecs::pack<const core::ecs::components::camera, const core::ecs::components::transform> cameras)
{
	if(!m_Surface->open() || !m_Swapchain->is_ready()) return core::ecs::command_buffer{state};
	size_t i = 0;
	for(auto[camera, transform] : cameras)
	{
		update_buffer(i++, transform, camera);
	}
	return core::ecs::command_buffer{state};
}

core::ecs::command_buffer render::tick_draws(
	const core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
	core::ecs::pack<const core::ecs::components::transform, const core::ecs::components::renderable,
	core::ecs::on_combine<core::ecs::components::transform, core::ecs::components::renderable>>
	renderables,
	core::ecs::pack<const core::ecs::components::transform, const core::ecs::components::renderable,
	core::ecs::on_break<core::ecs::components::transform, core::ecs::components::renderable>>
	broken_renderables)
{
	{
		m_Pass.clear();
		auto& default_layer = m_DrawGroup.layer("default", 0);
		for(auto[transform, renderable] : renderables)
		{
			m_DrawGroup.add(default_layer, renderable.material).add(renderable.geometry);
		}

		for(auto[transform, renderable] : broken_renderables)
		{
			if(auto dCall = m_DrawGroup.get(default_layer, renderable.material))
			{
				dCall.value().get().remove(renderable.geometry.operator const psl::UID &());
			}
		}

		m_Pass.add(m_DrawGroup);
		m_Pass.build();
	}
	m_Pass.prepare();
	m_Pass.present();
	return core::ecs::command_buffer{state};
}

void render::update_buffer(size_t index, const transform& transform, const core::ecs::components::camera& camera)
{
	PROFILE_SCOPE(core::profiler)
	while(index >= fdatasegment.size())
	{
		fdatasegment.emplace_back(m_Buffer->reserve(sizeof(framedata)).value());
	}
	{
		vec3 position  = transform.position;
		vec3 direction = transform.rotation * vec3::forward;
		vec3 up		   = vec3::up;

		framedata fdata{};
		fdata.ScreenParams =
			psl::vec4((float)m_Surface->data().width(), (float)m_Surface->data().height(), camera.near, camera.far);

		fdata.projectionMatrix = math::perspective_projection(
			math::radians(camera.fov), (float)m_Surface->data().width() / (float)m_Surface->data().height(),
			camera.near, camera.far);
		fdata.clipMatrix = clip;

		fdata.viewMatrix  = math::look_at(position, position + direction, up);
		fdata.modelMatrix = mat4x4(1);
		fdata.viewPos	 = vec4(position, 1.0);
		fdata.viewDir	 = vec4(direction, 0.0);

		fdata.VP  = fdata.clipMatrix * fdata.projectionMatrix * fdata.viewMatrix;
		fdata.WVP = fdata.VP * fdata.modelMatrix;
		m_Buffer->commit({{&fdata, fdatasegment[index]}});
	}
}