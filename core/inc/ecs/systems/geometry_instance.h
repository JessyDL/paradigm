#pragma once
#include "psl/ecs/state.h"
#include "gfx/bundle.h"
#include "vk/geometry.h"
#include "resource/resource.hpp"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "psl/bytell_map.h"

namespace core::ecs::systems
{
	namespace details
	{
		struct renderer_sort
		{
			int operator()(const core::ecs::components::renderable& lhs,
						   const core::ecs::components::renderable& rhs) const noexcept
			{
				return lhs.bundle.uid() > rhs.bundle.uid() ? 2 : lhs.bundle.uid() < rhs.bundle.uid() ? -2 : lhs.geometry.uid() > rhs.geometry.uid()? 1 : lhs.geometry.uid() == rhs.geometry.uid()? 0: -1;
			}
		};
		struct geometry_instance
		{
			size_t startIndex;
			size_t count;
		};
	} // namespace details

	auto geometry_instance =
		[profiler = &core::profiler](
			psl::ecs::info& info,
			psl::ecs::pack<const core::ecs::components::renderable, const core::ecs::components::transform,
						   psl::ecs::order_by<details::renderer_sort, core::ecs::components::renderable>>
				geometry_pack) {
			using namespace core::resource;
			using namespace core::gfx;
			using namespace core::os;
			using namespace core;
			using namespace core::ecs;
			using namespace core::ecs::systems;
			using namespace core::ecs::components;
			using namespace psl::math;

			size_t uniqueInstructions{0};
			profiler->scope_begin("release_all");
			for(auto [renderable, transform] : geometry_pack)
			{
				renderable.bundle.make_shared()->release_all();
			}
			profiler->scope_end();
			
			profiler->scope_begin("mapping");
			psl::bytell_map<psl::UID, psl::bytell_map<psl::UID, details::geometry_instance>> UniqueCombinations;

			for(uint32_t i = 0; i < (uint32_t)geometry_pack.size(); ++i)
			{
				const auto& renderer = std::get<const renderable>(geometry_pack[i]);
				if(UniqueCombinations[renderer.bundle].find(renderer.geometry) == std::end(UniqueCombinations[renderer.bundle]))
					UniqueCombinations[renderer.bundle].emplace(renderer.geometry, details::geometry_instance{i, 0});
				UniqueCombinations[renderer.bundle][renderer.geometry].count += 1;
			}
			profiler->scope_end();

			profiler->scope_begin("create_all");
			std::vector<psl::mat4x4> modelMats;
			for(const auto& unique_bundle : UniqueCombinations)
			{
				modelMats.clear();

				for (const auto& [geometryUID, geometryData] : unique_bundle.second)
				{
					const auto& renderer =
					std::get<const renderable>(geometry_pack[geometryData.startIndex]);
					auto bundleHandle   = renderer.bundle;
					auto geometryHandle = renderer.geometry;
					
					auto instancesID = bundleHandle->instantiate(geometryHandle, (uint32_t)geometryData.count);

					size_t indicesCompleted = 0;
					for(auto [startIndex, endIndex] : instancesID)
					{
						for (auto i = indicesCompleted; i < endIndex; ++i, ++indicesCompleted)
						{
							const auto& transform =
								std::get<const core::ecs::components::transform>(geometry_pack[i + geometryData.startIndex]);
							const psl::mat4x4 translationMat = translate(transform.position);
							const psl::mat4x4 rotationMat	= to_matrix(transform.rotation);
							const psl::mat4x4 scaleMat		 = scale(transform.scale);
							modelMats.emplace_back(translationMat * rotationMat * scaleMat);
						}

						bundleHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
					
						modelMats.clear();
					}
				}

			}
			/*psl::bytell_map<psl::UID, psl::bytell_map<psl::UID, std::vector<size_t>>> UniqueCombinations;

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
					if(uniqueIndex.second.size() == 0) continue;

					modelMats.clear();
					auto& renderer		= std::get<const renderable&>(geometry_pack[uniqueIndex.second[0]]);
					auto materialHandle = renderer.bundle.handle();
					auto geometryHandle = renderer.geometry.handle();

					auto instanceID = materialHandle->instantiate(geometryHandle)[0].first;

					uint32_t startIndex = instanceID;
					uint32_t indexCount = 0u;

					{
						++indexCount;
						auto& transform =
							std::get<const core::ecs::components::transform&>(geometry_pack[uniqueIndex.second[0]]);
						const psl::mat4x4 translationMat = translate(transform.position);
						const psl::mat4x4 rotationMat	= to_matrix(transform.rotation);
						const psl::mat4x4 scaleMat		 = scale(transform.scale);

						modelMats.emplace_back(translationMat * rotationMat * scaleMat);
					}

					for(auto i = std::next(std::begin(uniqueIndex.second)); i != std::end(uniqueIndex.second); ++i)
					{
						if(instanceID = materialHandle->instantiate(geometryHandle)[0].first; instanceID)
						{
							if((instanceID - indexCount != startIndex))
							{
								materialHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
								modelMats.clear();
								startIndex = instanceID;
								indexCount = 0;
							}


							++indexCount;
							auto& transform = std::get<const core::ecs::components::transform&>(geometry_pack[*i]);
							const psl::mat4x4 translationMat = translate(transform.position);
							const psl::mat4x4 rotationMat	= to_matrix(transform.rotation);
							const psl::mat4x4 scaleMat		 = scale(transform.scale);

							modelMats.emplace_back(translationMat * rotationMat * scaleMat);
						}
					}
					if(modelMats.size() > 0)
						materialHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
				}
				profiler->scope_end();
			}
			*/
			profiler->scope_end();

			core::log->info("this frame we ran {} unique instructions", uniqueInstructions);
		};

} // namespace core::ecs::systems