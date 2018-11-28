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
	state.register_dependency(*this, {m_Entities, m_Transforms, m_Renderers, m_Velocity});
	state.register_dependency(*this, {m_LifeEntities, m_Lifetime});
	state.register_dependency(*this, {m_CamEntities, m_CamTransform, core::ecs::filter<core::ecs::components::input_tag>{}});
}

//void tick(core::ecs::state& state, std::chrono::duration<float> dTime,
//		  pack<const entity, on_combine<const transform, const renderable>, except<lifetime>> pack_1,
//		  pack<lifetime, const renderable> pack_2)
//{
//
//}
float accTime {0.0f};

void geometry_instance::tick(core::ecs::state& state, std::chrono::duration<float> dTime)
{
	//pack<const entity, const transform, const renderable> pack{};
	//auto packet = std::begin(pack);
	//
	//auto tr = packet.get<const renderable>();
	//auto res{packet.get<0>()};
	//static_assert(std::is_same<decltype(packet.get<0>()), const entity&>::value);

	////auto [elementE, elementT, elementR] = *packet;

	//for(const auto[elementE, elementT, elementR] : pack)
	//{
	//
	//}

	//component_pack<transform, renderable, on_add<float>> pack();
	using pack_internal_t = typename component_pack<transform, renderable, on_combine<float, int>>::combine_t;
	pack_internal_t pack_instance;
	std::vector<int> iVec{0, 5, 3};
	std::vector<float> fVec{0.0f, 5.0f, 3.0f};
	pack<int, const float> p{iVec, fVec};

	for(auto [i, f] : p)
	{
		i += 1;
		i += 1;
		i += 1;
		// f *= 2.0f;
	}

	//auto pAck = p.read();
	//auto transformPack = p.get<const transform>();
	PROFILE_SCOPE(core::profiler)
	accTime += dTime.count();

	std::vector<entity> dead_ents;
	for(size_t i = 0; i < m_LifeEntities.size(); ++i)
	{
		m_Lifetime[i].value -= dTime.count();
		if(m_Lifetime[i].value <= 0.0f)
			dead_ents.emplace_back(m_LifeEntities[i]);
	}
	state.add_component<dead_tag>(dead_ents);

	core::profiler.scope_begin("release material handles");
	for(const auto& renderer : m_Renderers)
	{
		renderer.material.handle()->release_all();
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
	for(size_t i = 0; i < m_Entities.size(); ++i)
	{
		m_Transforms[i].position += m_Velocity[i].direction * m_Velocity[i].force * dTime.count();
		const auto mag = magnitude(m_Transforms[i].position - m_CamTransform[0].position);
		m_Transforms[i].rotation = normalize(psl::quat(0.8f* dTime.count() * saturate((mag - 6)*0.1f), 0.0f, 0.0f, 1.0f) * m_Transforms[i].rotation);
	}
	core::profiler.scope_end();
	
	ska::bytell_hash_map<psl::UID, ska::bytell_hash_map<psl::UID, std::vector<size_t>>> UniqueCombinations;
	for(size_t i = 0; i < m_Entities.size(); ++i)
	{
		UniqueCombinations[m_Renderers[i].material][m_Renderers[i].geometry].emplace_back(i);
	}

	std::vector<psl::mat4x4> modelMats;
	for(const auto& uniqueCombination : UniqueCombinations)
	{
		for(const auto& uniqueIndex : uniqueCombination.second)
		{
			if(uniqueIndex.second.size() == 0)
				continue;

			modelMats.clear();
			auto materialHandle = m_Renderers[uniqueIndex.second[0]].material.handle();
			auto geometryHandle = m_Renderers[uniqueIndex.second[0]].geometry.handle();

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
					const psl::mat4x4 translationMat = translate(m_Transforms[i].position);
					const psl::mat4x4 rotationMat = to_matrix(m_Transforms[i].rotation);
					const psl::mat4x4 scaleMat = scale(m_Transforms[i].scale);

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