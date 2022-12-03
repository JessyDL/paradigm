#include <stdint.h>

#include "ecs/systems/geometry_instance.hpp"
#include "ecs/components/renderable.hpp"
#include "ecs/components/transform.hpp"
#include "gfx/bundle.hpp"
#include "gfx/geometry.hpp"
#include "resource/resource.hpp"

using namespace core::ecs::systems;
using namespace psl;
using namespace psl::ecs;
using namespace psl::math;

using namespace core::resource;
using namespace core::ecs::components;

geometry_instancing::geometry_instancing(psl::ecs::state_t& state)
{
	state.declare<"geometry_instancing::static_add">(psl::ecs::threading::seq, &geometry_instancing::static_add, this);
	state.declare<"geometry_instancing::static_remove">(
	  psl::ecs::threading::seq, &geometry_instancing::static_remove, this);
	state.declare<"geometry_instancing::dynamic_system">(
	  psl::ecs::threading::seq, &geometry_instancing::dynamic_system, this);
	state.declare<"geometry_instancing::static_geometry_add">(
	  psl::ecs::threading::seq, &geometry_instancing::static_geometry_add, this);
	state.declare<"geometry_instancing::static_geometry_remove">(
	  psl::ecs::threading::seq, &geometry_instancing::static_geometry_remove, this);
}


void geometry_instancing::dynamic_system(
  info_t& info,
  pack<renderable, const transform, const dynamic_tag, except<dont_render_tag>, order_by<renderer_sort, renderable>>
	geometry_pack)
{
	// todo clean up in case the last renderable from a dynamic object is despawned. The instance will not be released
	// todo this will trash static instances as well
	core::log->warn("todo: instance leak");
	core::profiler.scope_begin("release_all");

	{
		auto renderers = geometry_pack.get<renderable>();
		for(auto renderable : renderers)
			if(renderable.bundle) renderable.bundle->release_all();

	}

	core::profiler.scope_end();

	core::profiler.scope_begin("mapping");
	std::unordered_map<psl::UID, std::unordered_map<psl::UID, geometry_instance>> UniqueCombinations;

	for(uint32_t i = 0; i < (uint32_t)geometry_pack.size(); ++i)
	{
		const auto& renderer = std::get<renderable&>(geometry_pack[i]);
		if(!renderer.bundle) continue;
		if(UniqueCombinations[renderer.bundle].find(renderer.geometry) == std::end(UniqueCombinations[renderer.bundle]))
		{
			UniqueCombinations[renderer.bundle].emplace(renderer.geometry, geometry_instance {i, 0});
		}
		UniqueCombinations[renderer.bundle][renderer.geometry].count += 1;
	}
	core::profiler.scope_end();

	core::profiler.scope_begin("create_all");
	std::vector<psl::mat4x4> modelMats;
	for(const auto& unique_bundle : UniqueCombinations)
	{
		modelMats.clear();

		for(const auto& [geometryUID, geometryData] : unique_bundle.second)
		{
			const auto& renderer = std::get<renderable&>(geometry_pack[geometryData.startIndex]);
			auto bundleHandle	 = renderer.bundle;
			auto geometryHandle	 = renderer.geometry;

			auto instancesID = bundleHandle->instantiate(geometryHandle, (uint32_t)geometryData.count);

			size_t indicesCompleted = 0;
			for(auto [startIndex, endIndex] : instancesID)
			{
				auto range = endIndex - startIndex;
				for(auto i = 0u; i < range; ++i, ++indicesCompleted)
				{
					const auto& transform = std::get<const core::ecs::components::transform&>(
					  geometry_pack[indicesCompleted + geometryData.startIndex]);
					const psl::mat4x4 translationMat = translate(transform.position);
					const psl::mat4x4 rotationMat	 = to_matrix(transform.rotation);
					const psl::mat4x4 scaleMat		 = scale(transform.scale);
					modelMats.emplace_back(translationMat * rotationMat * scaleMat);
				}

				if(!bundleHandle->set(
					 geometryHandle, startIndex, psl::string {core::gfx::constants::INSTANCE_MODELMATRIX}, modelMats))
					core::log->error(
					  "could not set the instance data for the dynamic elements in geometry: {} startIndex: {} size: "
					  "{}",
					  geometryHandle,
					  startIndex,
					  modelMats.size());
				modelMats.clear();
			}
		}
	}
	core::profiler.scope_end();
}

void geometry_instancing::static_add(info_t& info,
									 pack<entity,
										  const renderable,
										  const transform,
										  psl::ecs::except<dynamic_tag>,
										  on_combine<const renderable, const transform>,
										  order_by<renderer_sort, renderable>> geometry_pack)
{
	if(geometry_pack.size() == 0) return;

	core::profiler.scope_begin("mapping");
	std::unordered_map<psl::UID, std::unordered_map<psl::UID, geometry_instance>> UniqueCombinations;

	for(uint32_t i = 0; i < (uint32_t)geometry_pack.size(); ++i)
	{
		const auto& renderer = std::get<const renderable&>(geometry_pack[i]);
		if(!renderer.bundle) continue;
		if(UniqueCombinations[renderer.bundle].find(renderer.geometry) == std::end(UniqueCombinations[renderer.bundle]))
			UniqueCombinations[renderer.bundle].emplace(renderer.geometry, geometry_instance {i, 0});
		UniqueCombinations[renderer.bundle][renderer.geometry].count += 1;
	}
	core::profiler.scope_end();

	core::profiler.scope_begin("create_all");
	std::vector<psl::mat4x4> modelMats;
	psl::array<entity> eIds;
	eIds.resize(1);
	for(const auto& unique_bundle : UniqueCombinations)
	{
		modelMats.clear();

		for(const auto& [geometryUID, geometryData] : unique_bundle.second)
		{
			const auto& renderer = std::get<const renderable&>(geometry_pack[geometryData.startIndex]);
			auto bundleHandle	 = renderer.bundle;
			auto geometryHandle	 = renderer.geometry;

			auto instancesID = bundleHandle->instantiate(geometryHandle, (uint32_t)geometryData.count);

			uint32_t indicesCompleted = 0;
			for(auto [startIndex, endIndex] : instancesID)
			{
				for(auto i = startIndex; i < endIndex; ++i, ++indicesCompleted)
				{
					const auto& transform = std::get<const core::ecs::components::transform&>(
					  geometry_pack[indicesCompleted + geometryData.startIndex]);
					const psl::mat4x4 translationMat = translate(transform.position);
					const psl::mat4x4 rotationMat	 = to_matrix(transform.rotation);
					const psl::mat4x4 scaleMat		 = scale(transform.scale);
					modelMats.emplace_back(translationMat * rotationMat * scaleMat);

					eIds[0] = std::get<entity&>(geometry_pack[indicesCompleted + geometryData.startIndex]);
					info.command_buffer.add_components<instance_id>(eIds, instance_id {i});
				}
				bundleHandle->set(
				  geometryHandle, startIndex, psl::string {core::gfx::constants::INSTANCE_MODELMATRIX}, modelMats);

				modelMats.clear();
			}
		}
	}
	core::profiler.scope_end();
}
void geometry_instancing::static_remove(info_t& info,
										pack<entity,
											 renderable,
											 const instance_id,
											 psl::ecs::except<dynamic_tag>,
											 on_break<const renderable, const transform>> geometry_pack)
{
	if(geometry_pack.size() == 0) return;
	core::log->info("deallocating {} static instances", geometry_pack.size());
	core::profiler.scope_begin("release static geometry");
	for(auto [entity, renderable, instance_id] : geometry_pack)
	{
		if(renderable.bundle) renderable.bundle->release(renderable.geometry, instance_id.id);
	}

	info.command_buffer.remove_components<instance_id>(geometry_pack.get<entity>());
	core::profiler.scope_end();
}

void geometry_instancing::static_geometry_add(psl::ecs::info_t& info,
											  psl::ecs::pack<psl::ecs::entity,
															 const core::ecs::components::renderable,
															 psl::ecs::except<core::ecs::components::transform>,
															 psl::ecs::on_add<core::ecs::components::renderable>> pack)
{
	if(pack.size() == 0) return;
	psl::array<entity> eIds;
	eIds.resize(1);
	for(auto [entity, render] : pack)
	{
		auto bundleHandle	= render.bundle;
		auto geometryHandle = render.geometry;

		auto instancesID = bundleHandle->instantiate(geometryHandle, 1);
		eIds[0]			 = entity;
		info.command_buffer.add_components<instance_id>(eIds, instance_id {instancesID[0].first});
	}
}


void geometry_instancing::static_geometry_remove(
  psl::ecs::info_t& info,
  psl::ecs::pack<psl::ecs::entity,
				 core::ecs::components::renderable,
				 const instance_id,
				 psl::ecs::except<core::ecs::components::transform>,
				 psl::ecs::on_remove<core::ecs::components::renderable>> pack)
{
	if(pack.size() == 0) return;
	for(auto [entity, renderable, instance_id] : pack)
	{
		if(renderable.bundle) renderable.bundle->release(renderable.geometry, instance_id.id);
	}

	info.command_buffer.remove_components<instance_id>(pack.get<entity>());
}