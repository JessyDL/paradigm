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

render::render(core::resource::cache& cache,
	core::resource::handle<core::gfx::context> context, 
	core::resource::handle<core::gfx::swapchain> swapchain, 
	core::resource::handle<core::os::surface> surface,
	core::resource::handle<core::gfx::buffer> buffer) 
	: m_Cache(cache), m_Pass(context, swapchain), m_Swapchain(swapchain), m_Surface(surface), fdatasegment(buffer->reserve(sizeof(framedata)).value()), m_Buffer(buffer)
{
	m_Camera.far = 24.f;
	m_Camera.near = 0.1f;
	m_Camera.fov = 60;
	//update_buffer();
}

void render::announce(core::ecs::state& state)
{
	state.register_dependency(*this, {m_RenderableEntities, m_Transforms, m_Renderers });
	state.register_dependency(*this, {m_CameraEntities, m_Cameras});
}

void render::tick(core::ecs::state& state, std::chrono::duration<float> dTime)
{
	if (!m_Surface->open() || !m_Swapchain->is_ready())
		return;

	auto eCam = state.filter<transform, camera>();
	update_buffer(state.get_component<transform>(eCam[0]));

	m_Pass.clear();
	core::gfx::drawgroup dGroup{};
	auto& default_layer = dGroup.layer("default", 0);
	for (size_t i = 0; i < m_RenderableEntities.size(); ++i)
	{
		const auto& renderable = m_Renderers[i];
		auto mat = m_Cache.find<core::gfx::material>(renderable.material);
		auto geom = m_Cache.find<core::gfx::geometry>(renderable.geometry);
		dGroup.add(default_layer, mat).add(geom);
	}
	m_Pass.add(dGroup);
	m_Pass.build();
	m_Pass.prepare();
	m_Pass.present();
}

void render::update_buffer(const transform& camTransform)
{
	glm::vec3 position = glm::vec3(camTransform.position[0], camTransform.position[1], camTransform.position[2]);
	glm::vec3 direction = glm::vec3(camTransform.direction[0], camTransform.direction[1], camTransform.direction[2]);
	glm::vec3 up = glm::vec3(camTransform.up[0], camTransform.up[1], camTransform.up[2]);
	fdata.ScreenParams =
		glm::vec4((float)m_Surface->data().width(), (float)m_Surface->data().height(), m_Camera.near, m_Camera.far);

	fdata.projectionMatrix = glm::perspective(
		glm::radians(m_Camera.fov), (float)m_Surface->data().width() / (float)m_Surface->data().height(), m_Camera.near, m_Camera.far);
	fdata.clipMatrix = clip;

	fdata.viewMatrix = glm::lookAt(position, position + direction, up);
	fdata.modelMatrix = glm::mat4();
	fdata.viewPos	 = glm::vec4(position, 1.0);
	fdata.viewDir	 = glm::vec4(direction, 0.0);

	fdata.VP  = fdata.clipMatrix * fdata.projectionMatrix * fdata.viewMatrix;
	fdata.WVP = fdata.VP * fdata.modelMatrix;
	m_Buffer->commit({{&fdata, fdatasegment}});
}