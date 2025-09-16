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

psl::array<geometry_instancing::instance_id>
geometry_instancing::make_instances(renderable const& renderable, const transform* first, const transform* last) {
	const auto count = psl::narrow_cast<core::gfx::instancing_size_type>(std::distance(first, last));
	if(!renderable.bundle || !renderable.geometry || count == 0) {
		return {};
	}
	auto bundle		  = renderable.bundle;
	auto instancesIDs = bundle->instantiate(renderable.geometry, count);
	psl::array<psl::mat4x4> modelMats {};
	modelMats.reserve(count);
	psl::array<instance_id> compInstanceIDs {};
	compInstanceIDs.reserve(count);

	for(auto instanceId : instancesIDs) {
		const psl::mat4x4 translationMat = translate(first->position);
		const psl::mat4x4 rotationMat	 = to_matrix(first->rotation);
		const psl::mat4x4 scaleMat		 = scale(first->scale);
		modelMats.emplace_back(translationMat * rotationMat * scaleMat);
		compInstanceIDs.emplace_back(instance_id {instanceId});
		++first;
	}

	{
		std::scoped_lock lock(m_Mutex);
		if(!bundle->set(
			 renderable.geometry, instancesIDs, core::gfx::constants::INSTANCE_MODELMATRIX, std::move(modelMats)))
			core::log->error(
			  "could not set the instance data for the dynamic elements in geometry: {} size: "
			  "{}",
			  renderable.geometry,
			  instancesIDs.size());
	}
	return compInstanceIDs;
}

void geometry_instancing::dynamic_add(info_t& info,
									  pack_indirect_partial_t<entity_t,
															  const renderable,
															  const transform,
															  filter<dynamic_tag>,
															  except<dont_render_tag>,
															  on_combine<renderable, transform>> geometry_pack) {
	if(geometry_pack.size() == 0) {
		return;
	}

	transform const* first {&geometry_pack.get<const transform>()[0]};
	transform const* last {nullptr};
	auto* render_it		= &geometry_pack.get<renderable const>()[0];
	entity_t* first_ent = &geometry_pack.get<entity_t>()[0];
	entity_t* last_ent {nullptr};
	for(auto [ent, render, trns] : geometry_pack) {
		auto bundleHandle = render.bundle;
		last_ent		  = &ent;
		last			  = &trns;
		if(bundleHandle.uid() != render_it->bundle.uid() || render_it->geometry.uid() != render.geometry.uid()) {
			auto instanceComponents = make_instances(*render_it, first, last + 1);
			info.command_buffer.add_components<instance_id>(psl::array_view<entity_t>(first_ent, last_ent + 1),
															instanceComponents);
			first	  = &trns;
			first_ent = &ent;
			render_it = &render;
		}
	}

	auto instanceComponents = make_instances(*render_it, first, last + 1);
	info.command_buffer.add_components<instance_id>(psl::array_view<entity_t>(first_ent, last_ent + 1),
													instanceComponents);
}

void geometry_instancing::dynamic_remove(
  info_t& info,
  pack_indirect_full_t<entity_t,
					   const renderable,
					   const instance_id,
					   filter<dynamic_tag>,
					   except<dont_render_tag>,
					   on_break<renderable, transform, instance_id>,
					   psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>> pack) {
	if(pack.empty()) {
		return;
	}
	instance_id const* first {&pack.get<const instance_id>()[0]};
	instance_id const* last {nullptr};
	auto* render_it		= &pack.get<renderable const>()[0];
	entity_t* first_ent = &pack.get<entity_t>()[0];
	entity_t* last_ent {nullptr};

	psl::array<std::uint32_t> instanceIDs {};
	std::unordered_map<psl::UID, core::resource::handle<core::gfx::bundle>> seenBundles;

	for(auto [ent, render, instance_id] : pack) {
		auto bundleHandle = render.bundle;
		last_ent		  = &ent;
		last			  = &instance_id;
		instanceIDs.push_back(instance_id.id);
		if(bundleHandle.uid() != render_it->bundle.uid() || render_it->geometry.uid() != render.geometry.uid()) {
			auto scoped_lock = std::scoped_lock(m_Mutex);
			bundleHandle->release(render.geometry, instanceIDs);
			if(seenBundles.find(bundleHandle.uid()) == seenBundles.end()) {
				seenBundles.insert({bundleHandle.uid(), bundleHandle});
			}
			first	  = &instance_id;
			first_ent = &ent;
			render_it = &render;
			instanceIDs.clear();
		}
	}

	{
		auto scoped_lock = std::scoped_lock(m_Mutex);
		auto bundle		 = render_it->bundle;
		bundle->release(render_it->geometry, instanceIDs);
		if(seenBundles.find(bundle.uid()) == seenBundles.end()) {
			seenBundles.insert({bundle.uid(), bundle});
		}
	}

	for(auto& [uid, bundle] : seenBundles) {
		bundle->apply();
	}

	info.command_buffer.remove_components<instance_id>(pack.get<entity_t>().to_array());
}

void geometry_instancing::dynamic_update(info_t& info,
										 pack_indirect_partial_t<const renderable,
																 const transform,
																 const instance_id,
																 filter<dynamic_tag>,
																 except<dont_render_tag>,
																 psl::ecs::order_by<renderer_sort, renderable>> pack) {
	if(pack.empty()) {
		return;
	}
	psl::UID lastGeometryUID {};
	renderable const* lastRenderable = nullptr;

	psl::array<psl::mat4x4> modelMats {};
	psl::array<std::uint32_t> instanceIDs {};
	for(auto [r, t, id] : pack) {
		if(!r.bundle || !r.geometry) {
			continue;
		}
		if((lastRenderable == nullptr || lastRenderable->bundle.uid() != r.bundle.uid()) ||
		   lastGeometryUID != r.geometry.uid()) {
			if(lastRenderable) {
				auto scoped_lock = std::scoped_lock(m_Mutex);
				auto bundle		 = lastRenderable->bundle;
				bundle->set(lastRenderable->geometry,
							instanceIDs,
							core::gfx::constants::INSTANCE_MODELMATRIX,
							std::move(modelMats));
			}
			lastRenderable	= &r;
			lastGeometryUID = r.geometry.uid();
			modelMats.clear();
			instanceIDs.clear();
		}
		const psl::mat4x4 translationMat = translate(t.position);
		const psl::mat4x4 rotationMat	 = to_matrix(t.rotation);
		const psl::mat4x4 scaleMat		 = scale(t.scale);
		const auto modelMat				 = translationMat * rotationMat * scaleMat;
		modelMats.emplace_back(modelMat);
		instanceIDs.push_back(id.id);
	}
	if(lastRenderable) {
		auto scoped_lock = std::scoped_lock(m_Mutex);
		auto bundle		 = lastRenderable->bundle;
		bundle->set(
		  lastRenderable->geometry, instanceIDs, core::gfx::constants::INSTANCE_MODELMATRIX, std::move(modelMats));
	}
}

void geometry_instancing::static_add(info_t& info,
									 pack_direct_full_t<entity_t,
														const renderable,
														const transform,
														psl::ecs::except<dynamic_tag>,
														on_combine<const renderable, const transform>,
														order_by<renderer_sort, renderable>> geometry_pack) {
	if(geometry_pack.size() == 0) {
		return;
	}
	core::profiler.scope_begin("geometry_instancing::static_add");
	transform const* first {&geometry_pack.get<const transform>()[0]};
	transform const* last {nullptr};
	auto* render_it		= &geometry_pack.get<renderable const>()[0];
	entity_t* first_ent = &geometry_pack.get<entity_t>()[0];
	entity_t* last_ent {nullptr};
	for(auto [ent, render, trns] : geometry_pack) {
		auto bundleHandle = render.bundle;
		last_ent		  = &ent;
		last			  = &trns;
		if(bundleHandle.uid() != render_it->bundle.uid() || render_it->geometry.uid() != render.geometry.uid()) {
			auto instanceComponents = make_instances(*render_it, first, last + 1);
			info.command_buffer.add_components<instance_id>(psl::array_view<entity_t>(first_ent, last_ent + 1),
															instanceComponents);
			first	  = &trns;
			first_ent = &ent;
			render_it = &render;
		}
	}

	auto instanceComponents = make_instances(*render_it, first, last + 1);
	info.command_buffer.add_components<instance_id>(psl::array_view<entity_t>(first_ent, last_ent + 1),
													instanceComponents);
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
		if(renderable.bundle) {
			renderable.bundle->release(renderable.geometry, instance_id.id);
		}
	}

	info.command_buffer.remove_components<instance_id>(geometry_pack.get<entity_t>());
	core::profiler.scope_end();
}

void geometry_instancing::static_geometry_add(
  psl::ecs::info_t& info,
  psl::ecs::pack_indirect_full_t<psl::ecs::entity_t,
								 const core::ecs::components::renderable,
								 psl::ecs::except<core::ecs::components::transform>,
								 psl::ecs::on_add<core::ecs::components::renderable>,
								 psl::ecs::order_by<renderer_sort, core::ecs::components::renderable>> pack) {
	if(pack.size() == 0) {
		return;
	}

	auto instantiate = [](core::ecs::components::renderable const& rend, uint32_t count) {
		if(!rend.bundle || !rend.geometry || count == 0) {
			return psl::array<uint32_t> {};
		}
		auto bundle = rend.bundle;
		return bundle->instantiate(rend.geometry, count);
	};
	auto firstRenderable = &pack.get<const core::ecs::components::renderable>()[0];
	psl::UID geometry_uid {firstRenderable->geometry.uid()};
	psl::array<instance_id> instanceIds;
	uint32_t count {0};
	for(auto [entity, render] : pack) {
		auto bundleHandle	= render.bundle;
		auto geometryHandle = render.geometry;

		if(bundleHandle.uid() != firstRenderable->bundle.uid() || geometry_uid != geometryHandle.uid()) {
			if(count > 0) {
				auto instancesID = instantiate(*firstRenderable, count);
				for(auto instanceId : instancesID) {
					instanceIds.emplace_back(instance_id {instanceId});
				}
			}
			firstRenderable = &render;
			count			= 0;
			geometry_uid	= geometryHandle.uid();
		}
		++count;
	}
	if(count > 0 && firstRenderable) {
		auto instancesID = instantiate(*firstRenderable, count);
		for(auto instanceId : instancesID) {
			instanceIds.emplace_back(instance_id {instanceId});
		}
	}
	info.command_buffer.add_components<instance_id>(pack.get<entity_t>().to_array(), instanceIds);
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
