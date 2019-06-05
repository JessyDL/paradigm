#pragma once
#include "ecs/state.h"
#include "gfx/bundle.h"
#include "vk/geometry.h"
#include "systems/resource.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "bytell_map.h"

namespace core::ecs::systems
{
	auto geometry_instance =
		[profiler = &core::profiler](psl::ecs::info& info,
		   psl::ecs::pack<const core::ecs::components::renderable, const core::ecs::components::transform>
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

		profiler->scope_begin("release_all");
		for(auto[renderable, transform] : geometry_pack)
		{
			renderable.bundle.handle()->release_all();
		}
		profiler->scope_end();

		psl::bytell_map<psl::UID, psl::bytell_map<psl::UID, std::vector<size_t>>> UniqueCombinations;

		for(size_t i = 0; i < geometry_pack.size(); ++i)
		{
			auto& renderer = std::get<const renderable&>(geometry_pack[i]);
			UniqueCombinations[renderer.bundle][renderer.geometry].emplace_back(i);
		}

		profiler->scope_begin("create_all");
		std::vector<psl::mat4x4> modelMats;
		for(const auto& uniqueCombination : UniqueCombinations)
		{
			profiler->scope_begin("create_instance");
			for(const auto& uniqueIndex : uniqueCombination.second)
			{
				if(uniqueIndex.second.size() == 0)
					continue;

				modelMats.clear();
				auto& renderer = std::get<const renderable&>(geometry_pack[uniqueIndex.second[0]]);
				auto materialHandle = renderer.bundle.handle();
				auto geometryHandle = renderer.geometry.handle();

				auto instanceID = materialHandle->instantiate(geometryHandle);

				uint32_t startIndex = instanceID.value();
				uint32_t indexCount = 0u;

				{
					++indexCount;
					auto& transform = std::get<const core::ecs::components::transform&>(geometry_pack[uniqueIndex.second[0]]);
					const psl::mat4x4 translationMat = translate(transform.position);
					const psl::mat4x4 rotationMat = to_matrix(transform.rotation);
					const psl::mat4x4 scaleMat = scale(transform.scale);

					modelMats.emplace_back(translationMat * rotationMat * scaleMat);
				}

				for(auto i = std::next(std::begin(uniqueIndex.second)); i != std::end(uniqueIndex.second); ++i)
				{
					if(instanceID = materialHandle->instantiate(geometryHandle); instanceID)
					{
						if((instanceID.value() - indexCount != startIndex))
						{
							materialHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
							modelMats.clear();
							startIndex = instanceID.value();
							indexCount = 0;
						}


						++indexCount;
						auto& transform = std::get<const core::ecs::components::transform&>(geometry_pack[*i]);
						const psl::mat4x4 translationMat = translate(transform.position);
						const psl::mat4x4 rotationMat = to_matrix(transform.rotation);
						const psl::mat4x4 scaleMat = scale(transform.scale);

						modelMats.emplace_back(translationMat * rotationMat * scaleMat);
					}
				}
				if(modelMats.size() > 0)
					materialHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
			}
			profiler->scope_end();
		}
		profiler->scope_end();
	};

} // namespace core::ecs::systems