#include "stdafx.h"
#include "ecs/systems/geometry_instance.h"
#include "gfx/material.h"
#include "vk/geometry.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "ecs/components/input_tag.h"
#include "conversion_utils.h"
#include "ecs/pack.h"

using namespace core::resource;
using namespace core::gfx;
using namespace core::os;
using namespace core;
using namespace core::ecs;
using namespace core::ecs::systems;
using namespace core::ecs::components;
using namespace psl::math;



geometry_instance::geometry_instance(core::ecs::state& state)
{
	state.register_system(*this);
	state.register_dependency(*this, core::ecs::tick{}, m_Geometry);
	state.register_dependency(*this, core::ecs::tick{}, m_Cameras);
	state.register_dependency(*this, core::ecs::tick{}, m_Lifetimes);
}

float accTime {0.0f};

void geometry_instance::tick(core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime)
{
	PROFILE_SCOPE(core::profiler)
	accTime += dTime.count();

	std::vector<entity> dead_ents;
	for (auto [lifetime, entity] : m_Lifetimes)
	{
		lifetime.value -= dTime.count();
		if(lifetime.value <= 0.0f)
			dead_ents.emplace_back(entity);
	}

	state.add_component<dead_tag>(dead_ents);

	core::profiler.scope_begin("release material handles");
	for (auto [renderable, velocity, transform] : m_Geometry)
	{
		renderable.material.handle()->release_all();
	}
	core::profiler.scope_end();
	core::profiler.scope_begin("rotate and reposition all transforms");

	//for(size_t i = 0; i < m_Entities.size(); ++i)
	//{
	//	auto mag = magnitude(m_Transforms[i].position - m_CamTransform[0].position);
	//	m_Transforms[i].position += (normalize(m_Transforms[i].position) * dTime.count() * 3.0f * sin(accTime*0.1f));
	//	m_Transforms[i].rotation = normalize(psl::quat(0.8f* dTime.count() * saturate((mag - 6)*0.1f), 0.0f, 0.0f, 1.0f) * m_Transforms[i].rotation);
	//	if(mag < 6)
	//	{
	//		m_Transforms[i].rotation = normalize(psl::math::look_at_q(m_Transforms[i].position, m_CamTransform[0].position, psl::vec3::up));
	//	}

	//	//
	//	//m_Transforms[i].rotation = normalize(m_CamTransform[0].rotation);
	//}

	if (m_Cameras.size() == 0)
		return;


	auto& primary_camera = *std::begin(m_Cameras.get<const core::ecs::components::transform>());
	for (auto [renderable, velocity, transform] : m_Geometry)
	{
		transform.position += velocity.direction * velocity.force * dTime.count();
		const auto mag = magnitude(transform.position - primary_camera.position);
		transform.rotation = normalize(psl::quat(0.8f* dTime.count() * saturate((mag - 6)*0.1f), 0.0f, 0.0f, 1.0f) * transform.rotation);
	}
	core::profiler.scope_end();
	
	ska::bytell_hash_map<psl::UID, ska::bytell_hash_map<psl::UID, std::vector<size_t>>> UniqueCombinations;

	for(size_t i = 0; i < m_Geometry.size(); ++i)
	{
		auto& renderer = std::get<const renderable&>(m_Geometry[i]);
		UniqueCombinations[renderer.material][renderer.geometry].emplace_back(i);
	}

	std::vector<psl::mat4x4> modelMats;
	for(const auto& uniqueCombination : UniqueCombinations)
	{
		for(const auto& uniqueIndex : uniqueCombination.second)
		{
			if(uniqueIndex.second.size() == 0)
				continue;

			modelMats.clear();
			auto& renderer = std::get<const renderable&>(m_Geometry[uniqueIndex.second[0]]);
			auto materialHandle = renderer.material.handle();
			auto geometryHandle = renderer.geometry.handle();

			uint32_t startIndex = 0u;
			uint32_t indexCount = 0u;
			bool setStart = true;
			core::profiler.scope_begin("generate instance data");
			for(size_t i : uniqueIndex.second)
			{
				if(auto instanceID = materialHandle->instantiate(geometryHandle); instanceID)
				{
					if(!setStart && (instanceID.value() - indexCount != startIndex))
					{
						materialHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
						modelMats.clear();
						startIndex = instanceID.value();
						indexCount = 0;
					}

					if(setStart)
					{
						startIndex = instanceID.value();
					}

					++indexCount;
					setStart = false;
					auto& transform = std::get<core::ecs::components::transform&>(m_Geometry[i]);
					const psl::mat4x4 translationMat = translate(transform.position);
					const psl::mat4x4 rotationMat = to_matrix(transform.rotation);
					const psl::mat4x4 scaleMat = scale(transform.scale);

					modelMats.emplace_back(translationMat * rotationMat * scaleMat);
				}
			}
			core::profiler.scope_end();
			core::profiler.scope_begin("sending new instance data to GPU");
			if(modelMats.size() > 0)
				materialHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
			core::profiler.scope_end();
		}		
	}	
}