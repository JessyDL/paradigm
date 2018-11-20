#include "stdafx.h"
#include "ecs/systems/geometry_instance.h"
#include "gfx/material.h"
#include "vk/geometry.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "ecs/components/input_tag.h"
#include "conversion_utils.h"

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
	state.register_dependency(*this, {m_CamEntities, m_CamTransform, core::ecs::filter<core::ecs::components::input_tag>{}});
}

std::vector<psl::mat4x4> modelMats;
float accTime {0.0f};
void geometry_instance::tick(core::ecs::state& state, std::chrono::duration<float> dTime)
{
	accTime += dTime.count();

	for(const auto& renderer : m_Renderers)
	{
		renderer.material.handle()->release_all();
	}
	for(size_t i = 0; i < m_Entities.size(); ++i)
	{
		m_Transforms[i].position += (normalize(m_Transforms[i].position) * dTime.count() * 3.0f * sin(accTime*0.1f));
		m_Transforms[i].rotation = normalize(psl::math::look_at_q(m_Transforms[i].position, m_CamTransform[0].position, psl::vec3::up));
		//m_Transforms[i].rotation = normalize(m_CamTransform[0].rotation);
	}

	modelMats.clear();

	handle<material> cachedMat;
	handle<geometry> cachedGeom;
	uint32_t startIndex = 0u;
	uint32_t indexCount = 0u;
	bool setStart = true;
	for(size_t i = 0; i < m_Entities.size(); ++i)
	{
		if(auto index = m_Renderers[i].material.handle()->instantiate(m_Renderers[i].geometry); index)
		{
			if(i > 0 && 
				(m_Renderers[i].material.handle() != cachedMat ||
				 m_Renderers[i].geometry.handle() != cachedGeom || 
				 index.value() - indexCount != startIndex))
			{
				cachedMat->set(cachedGeom, startIndex, "INSTANCE_TRANSFORM", modelMats);
				modelMats.clear();
				cachedMat = m_Renderers[i].material.handle();
				cachedGeom = m_Renderers[i].geometry.handle();
				startIndex = index.value();
				indexCount = 0;
			}

			if(setStart)
			{
				startIndex = index.value();
				cachedMat = m_Renderers[i].material.handle();
				cachedGeom = m_Renderers[i].geometry.handle();
			}
			++indexCount;
			setStart = false;
			psl::mat4x4 translationMat = translate(m_Transforms[i].position);
			psl::mat4x4 rotationMat = to_matrix(m_Transforms[i].rotation);
			psl::mat4x4 scaleMat = scale(m_Transforms[i].scale);

			modelMats.emplace_back(translationMat * rotationMat * scaleMat);

			//m_Renderers[i].material.handle()->set(m_Renderers[i].geometry, index.value(), "INSTANCE_TRANSFORM", modelMatrix);
		}
	}

	if(modelMats.size() > 0)
		cachedMat->set(cachedGeom, startIndex, "INSTANCE_TRANSFORM", modelMats);
	modelMats.clear();
	
}