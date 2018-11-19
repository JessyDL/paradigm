#include "stdafx.h"
#include "ecs/systems/render.h"
#include "gfx/drawgroup.h"
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

render::render(handle<context> context, handle<swapchain> swapchain, handle<surface> surface, handle<buffer> buffer)
	: m_Pass(context, swapchain), m_Swapchain(swapchain), m_Surface(surface), m_Buffer(buffer)
{}

void render::announce(ecs::state& state)
{
	state.register_dependency(*this, {m_RenderableEntities, m_Transforms, m_Renderers});
	state.register_dependency(*this, {m_CameraEntities, m_Cameras, m_CameraTransforms});
}

int first_run_renderer = 0;
void render::tick(ecs::state& state, std::chrono::duration<float> dTime)
{
	if(!m_Surface->open() || !m_Swapchain->is_ready()) return;

	for(size_t i = 0; i < m_CameraEntities.size(); ++i)
	{
		update_buffer(i, m_CameraTransforms[i], m_Cameras[i]);
	}

	//if(first_run_renderer < 3)
	{
		++first_run_renderer;
		m_Pass.clear();
		core::gfx::drawgroup dGroup{};
		auto& default_layer = dGroup.layer("default", 0);
		for(size_t i = 0; i < m_RenderableEntities.size(); ++i)
		{
			const auto& renderable = m_Renderers[i];
			dGroup.add(default_layer, renderable.material).add(renderable.geometry);
		}
		m_Pass.add(dGroup);
		m_Pass.build();
	}
	m_Pass.prepare();
	m_Pass.present();
}

void render::update_buffer(size_t index, const transform& transform, const core::ecs::components::camera& camera)
{
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