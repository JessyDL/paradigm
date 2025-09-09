#include <stdint.h>

#include "core/ecs/components/renderable.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/ecs/systems/geometry_instance.hpp"
#include "core/gfx/bundle.hpp"
#include "core/gfx/geometry.hpp"
#include "core/resource/resource.hpp"

using namespace core::ecs::systems;
using namespace psl;
using namespace psl::ecs;
using namespace psl::math;

using namespace core::resource;
using namespace core::ecs::components;

geometry_instancing::geometry_instancing(psl::ecs::state_t& state) {
	state.declare<"geometry_instancing::static_add">(psl::ecs::threading::seq, &geometry_instancing::static_add, this);
	state.declare<"geometry_instancing::static_remove">(
	  psl::ecs::threading::seq, &geometry_instancing::static_remove, this);
	/*state.declare<"geometry_instancing::dynamic_system">(
	  psl::ecs::threading::seq, &geometry_instancing::dynamic_system, this);*/
	state.declare<"geometry_instancing::static_geometry_add">(
	  psl::ecs::threading::seq, &geometry_instancing::static_geometry_add, this);
	state.declare<"geometry_instancing::static_geometry_remove">(
	  psl::ecs::threading::seq, &geometry_instancing::static_geometry_remove, this);
	state.declare<"geometry_instancing::dynamic_add">(
	  psl::ecs::threading::seq, &geometry_instancing::dynamic_add, this);
	state.declare<"geometry_instancing::dynamic_remove">(
	  psl::ecs::threading::par, &geometry_instancing::dynamic_remove, this);
	state.declare<"geometry_instancing::dynamic_update">(
	  psl::ecs::threading::par, &geometry_instancing::dynamic_update, this);
}


void geometry_instancing::dynamic_add(info_t& info,
									  pack_indirect_full_t<entity_t,
														   renderable,
														   const transform,
														   filter<dynamic_tag>,
														   except<dont_render_tag>,
														   on_combine<renderable, transform>> geometry_pack) {
	if(geometry_pack.size() == 0) {
		return;
	}

	auto instantiate = [&info](renderable& rend,
							   const transform* first,
							   const transform* last,
							   entity_t* first_ent,
							   entity_t* last_ent,
							   uint32_t count) {
		if(!rend.bundle || !rend.geometry || count == 0) {
			return;
		}
		auto instancesID = rend.bundle->instantiate(rend.geometry, count);
		psl::array<psl::mat4x4> modelMats {};
		modelMats.reserve(count);
		psl::array<instance_id> instanceIDs {};
		instanceIDs.reserve(count);

		for(auto [startIndex, endIndex] : instancesID) {
			auto const range = endIndex - startIndex;
			for(auto i = 0u; i < range; ++i, ++first) {
				const psl::mat4x4 translationMat = translate(first->position);
				const psl::mat4x4 rotationMat	 = to_matrix(first->rotation);
				const psl::mat4x4 scaleMat		 = scale(first->scale);
				modelMats.emplace_back(translationMat * rotationMat * scaleMat);
				instanceIDs.emplace_back(instance_id {startIndex + i});
			}

			if(!rend.bundle->set(rend.geometry, startIndex, core::gfx::constants::INSTANCE_MODELMATRIX, modelMats))
				core::log->error(
				  "could not set the instance data for the dynamic elements in geometry: {} startIndex: {} size: "
				  "{}",
				  rend.geometry,
				  startIndex,
				  modelMats.size());
			modelMats.clear();
		}
		info.command_buffer.add_components<instance_id>(psl::array_view<entity_t>(first_ent, last_ent), instanceIDs);
	};

	core::resource::handle<core::gfx::bundle> lastBundle {};
	uint32_t count {0};
	transform const* first {geometry_pack.get<const transform>().data()};
	transform const* last {nullptr};
	auto* render_it = geometry_pack.get<renderable>().data();
	psl::array<psl::mat4x4> modelMats {};
	psl::array<instance_id> instanceIDs {};
	entity_t* first_ent = geometry_pack.get<entity_t>().data();
	entity_t* last_ent {nullptr};
	for(auto [ent, render, trns] : geometry_pack) {
		auto bundleHandle = render.bundle;
		last_ent		  = &ent;
		last			  = &trns;
		render_it		  = &render;
		if(bundleHandle != lastBundle && lastBundle && bundleHandle && count > 0) {
			instantiate(*render_it, first, last + 1, first_ent, last_ent + 1, count);
			first	  = &trns;
			count	  = 0;
			first_ent = &ent;
		}

		lastBundle = bundleHandle;
		++count;
	}

	if(count > 0) {
		instantiate(*render_it, first, last + 1, first_ent, last_ent + 1, count);
	}
}

void geometry_instancing::dynamic_remove(info_t& info,
										 pack_direct_partial_t<entity_t,
															   renderable,
															   const instance_id,
															   filter<dynamic_tag>,
															   except<dont_render_tag>,
															   on_break<renderable, transform, instance_id>> pack) {
	for(auto [e, r, id] : pack) {
		if(r.bundle && r.geometry) {
			r.bundle->release(r.geometry, id.id);
		}
	}

	info.command_buffer.remove_components<instance_id>(pack.get<entity_t>());
}

void geometry_instancing::dynamic_update(info_t& info,
										 pack_indirect_partial_t<renderable,
																 const transform,
																 const instance_id,
																 filter<dynamic_tag>,
																 except<dont_render_tag>,
																 order_by<instance_id_sort, instance_id>> pack) {
	if(pack.empty()) {
		return;
	}
	struct temp {
		instance_id id;
		psl::mat4x4 model;
	};
	std::unordered_map<psl::UID, std::unordered_map<psl::UID, std::pair<renderable*, psl::array<temp>>>> combos;
	psl::UID lastBundleUID {};
	psl::UID lastGeometryUID {};
	std::pair<renderable*, psl::array<temp>>* currentArray = nullptr;
	for(auto [r, t, id] : pack) {
		const psl::mat4x4 translationMat = translate(t.position);
		const psl::mat4x4 rotationMat	 = to_matrix(t.rotation);
		const psl::mat4x4 scaleMat		 = scale(t.scale);
		const auto modelMat				 = translationMat * rotationMat * scaleMat;
		if(r.bundle.uid() == lastBundleUID && r.geometry.uid() == lastGeometryUID && currentArray) {
			currentArray->second.emplace_back(temp {id, modelMat});
		} else {
			lastBundleUID	= r.bundle.uid();
			lastGeometryUID = r.geometry.uid();
			currentArray	= &combos[r.bundle][r.geometry];
			currentArray->second.emplace_back(temp {id, modelMat});
			if(currentArray->first == nullptr) {
				currentArray->first = &r;
				currentArray->second.reserve(pack.size());
			}
		}
	}

	psl::array<psl::mat4x4> modelMats {};
	modelMats.reserve(pack.size());
	for(auto& [bundleUID, geomMap] : combos) {
		for(auto& [geometryUID, instanceData] : geomMap) {
			std::sort(instanceData.second.begin(), instanceData.second.end(), [](const temp& lhs, const temp& rhs) {
				return lhs.id.id < rhs.id.id;
			});

			size_t index	  = 0;
			size_t totalCount = 0;
			while(index < instanceData.second.size()) {
				totalCount = index;
				// figure out contiguous ranges
				auto* firstModelMat = &instanceData.second[index].model;
				auto firstId		= instanceData.second[index].id.id;
				modelMats.emplace_back(instanceData.second[index].model);
				while(index + 1 < instanceData.second.size() &&
					  firstId + index + 1 == instanceData.second[index + 1].id.id) {
					++index;
					modelMats.emplace_back(instanceData.second[index].model);
				}

				totalCount = index - totalCount + 1;

				auto scoped_lock   = std::scoped_lock(m_Mutex);
				auto* lastModelMat = &instanceData.second[index].model;
				instanceData.first->bundle->set(
				  instanceData.first->geometry, firstId, core::gfx::constants::INSTANCE_MODELMATRIX, modelMats);
				++index;
				modelMats.clear();
			}
		}
	}
}


void geometry_instancing::dynamic_system(info_t& info,
										 pack_direct_full_t<renderable,
															const transform,
															const dynamic_tag,
															except<dont_render_tag>,
															order_by<renderer_sort, renderable>> geometry_pack) {
	// todo clean up in case the last renderable from a dynamic object is despawned. The instance will not be released
	// todo this will trash static instances as well
	core::log->warn("todo: instance leak");
	core::profiler.scope_begin("release_all");

	{
		auto renderers = geometry_pack.get<renderable>();
		for(auto renderable : renderers) {
			if(renderable.bundle) {
				renderable.bundle->release_all();
			}
		}
	}

	core::profiler.scope_end();

	core::profiler.scope_begin("mapping");
	std::unordered_map<psl::UID, std::unordered_map<psl::UID, geometry_instance>> UniqueCombinations;

	for(uint32_t i = 0; i < (uint32_t)geometry_pack.size(); ++i) {
		const auto& renderer = std::get<renderable&>(geometry_pack[i]);
		if(!renderer.bundle)
			continue;
		if(UniqueCombinations[renderer.bundle].find(renderer.geometry) ==
		   std::end(UniqueCombinations[renderer.bundle])) {
			UniqueCombinations[renderer.bundle].emplace(renderer.geometry, geometry_instance {i, 0});
		}
		UniqueCombinations[renderer.bundle][renderer.geometry].count += 1;
	}
	core::profiler.scope_end();

	core::profiler.scope_begin("create_all");
	std::vector<psl::mat4x4> modelMats;
	for(const auto& unique_bundle : UniqueCombinations) {
		modelMats.clear();

		for(const auto& [geometryUID, geometryData] : unique_bundle.second) {
			const auto& renderer = std::get<renderable&>(geometry_pack[geometryData.startIndex]);
			auto bundleHandle	 = renderer.bundle;
			auto geometryHandle	 = renderer.geometry;

			auto instancesID = bundleHandle->instantiate(geometryHandle, (uint32_t)geometryData.count);

			size_t indicesCompleted = 0;
			for(auto [startIndex, endIndex] : instancesID) {
				auto range = endIndex - startIndex;
				for(auto i = 0u; i < range; ++i, ++indicesCompleted) {
					const auto& transform = std::get<const core::ecs::components::transform&>(
					  geometry_pack[indicesCompleted + geometryData.startIndex]);
					const psl::mat4x4 translationMat = translate(transform.position);
					const psl::mat4x4 rotationMat	 = to_matrix(transform.rotation);
					const psl::mat4x4 scaleMat		 = scale(transform.scale);
					modelMats.emplace_back(translationMat * rotationMat * scaleMat);
				}

				if(!bundleHandle->set(
					 geometryHandle, startIndex, core::gfx::constants::INSTANCE_MODELMATRIX, modelMats))
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
									 pack_direct_full_t<entity_t,
														const renderable,
														const transform,
														psl::ecs::except<dynamic_tag>,
														on_combine<const renderable, const transform>,
														order_by<renderer_sort, renderable>> geometry_pack) {
	if(geometry_pack.size() == 0)
		return;

	core::profiler.scope_begin("mapping");
	std::unordered_map<psl::UID, std::unordered_map<psl::UID, geometry_instance>> UniqueCombinations;

	for(uint32_t i = 0; i < (uint32_t)geometry_pack.size(); ++i) {
		const auto& renderer = std::get<const renderable&>(geometry_pack[i]);
		if(!renderer.bundle)
			continue;
		if(UniqueCombinations[renderer.bundle].find(renderer.geometry) == std::end(UniqueCombinations[renderer.bundle]))
			UniqueCombinations[renderer.bundle].emplace(renderer.geometry, geometry_instance {i, 0});
		UniqueCombinations[renderer.bundle][renderer.geometry].count += 1;
	}
	core::profiler.scope_end();

	core::profiler.scope_begin("create_all");
	std::vector<psl::mat4x4> modelMats;
	psl::array<entity_t> eIds;
	psl::array<instance_id> instanceIds;
	for(const auto& unique_bundle : UniqueCombinations) {
		modelMats.clear();

		for(const auto& [geometryUID, geometryData] : unique_bundle.second) {
			const auto& renderer = std::get<const renderable&>(geometry_pack[geometryData.startIndex]);
			auto bundleHandle	 = renderer.bundle;
			auto geometryHandle	 = renderer.geometry;

			auto instancesID = bundleHandle->instantiate(geometryHandle, (uint32_t)geometryData.count);

			uint32_t indicesCompleted = 0;
			for(auto [startIndex, endIndex] : instancesID) {
				eIds.reserve(endIndex - startIndex);
				instanceIds.reserve(endIndex - startIndex);
				modelMats.reserve(endIndex - startIndex);
				for(auto i = startIndex; i < endIndex; ++i, ++indicesCompleted) {
					const auto& transform = std::get<const core::ecs::components::transform&>(
					  geometry_pack[indicesCompleted + geometryData.startIndex]);
					const psl::mat4x4 translationMat = translate(transform.position);
					const psl::mat4x4 rotationMat	 = to_matrix(transform.rotation);
					const psl::mat4x4 scaleMat		 = scale(transform.scale);
					modelMats.emplace_back(translationMat * rotationMat * scaleMat);

					eIds.emplace_back(std::get<entity_t&>(geometry_pack[indicesCompleted + geometryData.startIndex]));
					instanceIds.emplace_back(instance_id {i});
				}
				info.command_buffer.add_components<instance_id>(eIds, instanceIds);
				bundleHandle->set(geometryHandle, startIndex, core::gfx::constants::INSTANCE_MODELMATRIX, modelMats);

				modelMats.clear();
				eIds.clear();
				instanceIds.clear();
			}
		}
	}
	core::profiler.scope_end();
}
void geometry_instancing::static_remove(info_t& info,
										pack_direct_full_t<entity_t,
														   renderable,
														   const instance_id,
														   psl::ecs::except<dynamic_tag>,
														   on_break<const renderable, const transform>> geometry_pack) {
	if(geometry_pack.size() == 0)
		return;
	core::log->info("deallocating {} static instances", geometry_pack.size());
	core::profiler.scope_begin("release static geometry");
	for(auto [entity, renderable, instance_id] : geometry_pack) {
		if(renderable.bundle)
			renderable.bundle->release(renderable.geometry, instance_id.id);
	}

	info.command_buffer.remove_components<instance_id>(geometry_pack.get<entity_t>());
	core::profiler.scope_end();
}

void geometry_instancing::static_geometry_add(
  psl::ecs::info_t& info,
  psl::ecs::pack_direct_full_t<psl::ecs::entity_t,
							   const core::ecs::components::renderable,
							   psl::ecs::except<core::ecs::components::transform>,
							   psl::ecs::on_add<core::ecs::components::renderable>> pack) {
	if(pack.size() == 0)
		return;
	psl::array<instance_id> instanceIds;
	for(auto [entity, render] : pack) {
		auto bundleHandle	= render.bundle;
		auto geometryHandle = render.geometry;

		auto instancesID = bundleHandle->instantiate(geometryHandle, 1);
		instanceIds.emplace_back(instancesID[0].first);
	}
	info.command_buffer.add_components<instance_id>(pack.get<entity_t>(), instanceIds);
}


void geometry_instancing::static_geometry_remove(
  psl::ecs::info_t& info,
  psl::ecs::pack_direct_full_t<psl::ecs::entity_t,
							   core::ecs::components::renderable,
							   const instance_id,
							   psl::ecs::except<core::ecs::components::transform>,
							   psl::ecs::on_remove<core::ecs::components::renderable>> pack) {
	if(pack.size() == 0)
		return;
	for(auto [entity, renderable, instance_id] : pack) {
		if(renderable.bundle)
			renderable.bundle->release(renderable.geometry, instance_id.id);
	}

	info.command_buffer.remove_components<instance_id>(pack.get<entity_t>());
}
