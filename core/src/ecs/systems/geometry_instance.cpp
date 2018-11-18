#include "stdafx.h"
#include "ecs/systems/geometry_instance.h"
#include "gfx/material.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"

using namespace core::resource;
using namespace core::gfx;
using namespace core::os;
using namespace core;
using namespace core::ecs::systems;
using namespace core::ecs::components;
using namespace psl::math;

void geometry_instance::announce(core::ecs::state& state)
{
	state.register_dependency(*this, {m_Entities, m_Transforms, m_Renderers});
}

void geometry_instance::tick(core::ecs::state& state, std::chrono::duration<float> dTime)
{
	for(const auto& renderer : m_Renderers)
	{
		renderer.material.handle()->release_all();
	}


	for(size_t i = 0; i < m_Entities.size(); ++i)
	{
		if(auto index = m_Renderers[i].material.handle()->instantiate(m_Renderers[i].geometry); index)
		{
			psl::mat4x4 modelMatrix;
			const psl::mat4x4 translationMat = translate(psl::mat4x4(1.0f), m_Transforms[i].position);
			const psl::mat4x4 rotationMat = to_matrix(m_Transforms[i].rotation);
			const psl::mat4x4 scaleMat = scale(psl::mat4x4(1.0f), m_Transforms[i].scale);

			modelMatrix = translationMat * scaleMat * rotationMat;

			m_Renderers[i].material.handle()->set(m_Renderers[i].geometry, index.value(), "INSTANCE_TRANSFORM", modelMatrix);
		}
	}
}