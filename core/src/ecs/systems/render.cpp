#include "stdafx.h"
#include "ecs/systems/render.h"
#include "ecs/components/transform.h"
#include "ecs/components/renderable.h"
#include "vk/swapchain.h"
#include "vk/context.h"
#include "os/surface.h"
#include "gfx/material.h"
#include "vk/geometry.h"

#include "vk/buffer.h"

using namespace core::ecs::systems;
using namespace core::ecs::components;

render::render(core::resource::handle<core::gfx::context> context, 
	core::resource::handle<core::gfx::swapchain> swapchain, 
	core::resource::handle<core::os::surface> surface,
	core::resource::handle<core::gfx::buffer> buffer) 
	: m_Pass(context, swapchain), m_Swapchain(swapchain), m_Surface(surface), m_Buffer(buffer)
{
}

void render::announce(core::ecs::state& state)
{
	state.register_dependency(*this, {m_RenderableEntities, m_Transforms, m_Renderers });
	state.register_dependency(*this, {m_CameraEntities, m_Cameras, m_CameraTransforms});
}

void render::tick(core::ecs::state& state, std::chrono::duration<float> dTime)
{
	if (!m_Surface->open() || !m_Swapchain->is_ready())
		return;

	for(size_t i = 0; i < m_CameraEntities.size(); ++i)
	{
		update_buffer(i, m_CameraTransforms[i], m_Cameras[i]);
	}

	m_Pass.clear();
	core::gfx::drawgroup dGroup{};
	auto& default_layer = dGroup.layer("default", 0);
	for (size_t i = 0; i < m_RenderableEntities.size(); ++i)
	{
		const auto& renderable = m_Renderers[i];
		dGroup.add(default_layer, renderable.material).add(renderable.geometry);
	}
	m_Pass.add(dGroup);
	m_Pass.build();
	m_Pass.prepare();
	m_Pass.present();
}

void render::update_buffer(size_t index, const transform& transform, const core::ecs::components::camera& camera)
{
	while(index >= fdatasegment.size())
	{
		fdatasegment.emplace_back(m_Buffer->reserve(sizeof(framedata)).value());
	}
	glm::vec3 position = glm::vec3(transform.position[0], transform.position[1], transform.position[2]);
	glm::vec3 direction = glm::vec3(transform.direction[0], transform.direction[1], transform.direction[2]);
	glm::vec3 up = glm::vec3(transform.up[0], transform.up[1], transform.up[2]);
	fdata.ScreenParams =
		glm::vec4((float)m_Surface->data().width(), (float)m_Surface->data().height(), camera.near, camera.far);

	fdata.projectionMatrix = glm::perspective(
		glm::radians(camera.fov), (float)m_Surface->data().width() / (float)m_Surface->data().height(), camera.near, camera.far);
	fdata.clipMatrix = clip;

	fdata.viewMatrix = glm::lookAt(position, position + direction, up);
	fdata.modelMatrix = glm::mat4();
	fdata.viewPos	 = glm::vec4(position, 1.0);
	fdata.viewDir	 = glm::vec4(direction, 0.0);

	fdata.VP  = fdata.clipMatrix * fdata.projectionMatrix * fdata.viewMatrix;
	fdata.WVP = fdata.VP * fdata.modelMatrix;
	m_Buffer->commit({{&fdata, fdatasegment[index]}});
}