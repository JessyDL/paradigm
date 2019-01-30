#pragma once
#include "gfx/material.h"
#include "vk/geometry.h"
#include "ecs/pack.h"
#include "systems/resource.h"
#include "ecs/command_buffer.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "bytell_map.h"

namespace core::ecs::systems
{
	auto geometry_instance = 
		[](const core::ecs::state& state, 
		   std::chrono::duration<float> dTime, 
		   std::chrono::duration<float> rTime, 
		   core::ecs::pack<const core::ecs::components::renderable, core::ecs::components::transform>
								geometry_pack)
	{
		using namespace core::resource;
		using namespace core::gfx;
		using namespace core::os;
		using namespace core;
		using namespace core::ecs;
		using namespace core::ecs::systems;
		using namespace core::ecs::components;
		using namespace psl::math;


		for(auto[renderable, transform] : geometry_pack)
		{
			renderable.material.handle()->release_all();
		}

		psl::bytell_map<psl::UID, psl::bytell_map<psl::UID, std::vector<size_t>>> UniqueCombinations;

		for(size_t i = 0; i < geometry_pack.size(); ++i)
		{
			auto& renderer = std::get<const renderable&>(geometry_pack[i]);
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
				auto& renderer = std::get<const renderable&>(geometry_pack[uniqueIndex.second[0]]);
				auto materialHandle = renderer.material.handle();
				auto geometryHandle = renderer.geometry.handle();

				uint32_t startIndex = 0u;
				uint32_t indexCount = 0u;
				bool setStart = true;
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
						auto& transform = std::get<core::ecs::components::transform&>(geometry_pack[i]);
						const psl::mat4x4 translationMat = translate(transform.position);
						const psl::mat4x4 rotationMat = to_matrix(transform.rotation);
						const psl::mat4x4 scaleMat = scale(transform.scale);

						modelMats.emplace_back(translationMat * rotationMat * scaleMat);
					}
				}
				if(modelMats.size() > 0)
					materialHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
			}
		}

		return core::ecs::command_buffer{state};
	};

} // namespace core::ecs::systems