#include "stdafx.h"
#include "ecs/systems/geometry_instance.h"
#include "gfx/material.h"
#include "vk/geometry.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
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
}

void geometry_instance::tick(core::ecs::commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime)
{
	PROFILE_SCOPE(core::profiler)
	
	core::profiler.scope_begin("release material handles");
	for (auto [renderable, transform] : m_Geometry)
	{
		renderable.material.handle()->release_all();
	}
	core::profiler.scope_end();
		
	ska::bytell_hash_map<psl::UID, ska::bytell_hash_map<psl::UID, std::vector<size_t>>> UniqueCombinations;

	core::profiler.scope_begin("find unique material and geometry combinations");
	for(size_t i = 0; i < m_Geometry.size(); ++i)
	{
		auto& renderer = std::get<const renderable&>(m_Geometry[i]);
		UniqueCombinations[renderer.material][renderer.geometry].emplace_back(i);
	}

	core::profiler.scope_end();
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