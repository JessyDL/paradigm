#include "core/ecs/systems/render.hpp"
#include "core/ecs/components/renderable.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/gfx/bundle.hpp"
#include "core/gfx/drawpass.hpp"
#include "core/gfx/geometry.hpp"
#include "core/gfx/render_graph.hpp"
#include "psl/ecs/order_by.hpp"

using core::resource::handle;
using namespace core::gfx;
using namespace core;
using namespace psl::ecs;
using namespace core::ecs::systems;
using namespace core::ecs::components;
using namespace psl;

using std::chrono::duration;

bool render::renderer_sort::operator()(const core::ecs::components::renderable& lhs,
									   const core::ecs::components::renderable& rhs) const noexcept {
	if(lhs.bundle.uid() != rhs.bundle.uid())
		return lhs.bundle.uid() < rhs.bundle.uid();
	else
		return lhs.geometry.uid() < rhs.geometry.uid();
}

render::render(state_t& state, psl::view_ptr<core::gfx::drawpass> pass) : m_Pass(pass) {
	state.declare<"render::update_instance_data">(threading::seq, &render::update_instance_data, this);
	state.declare<"render::update_instance_object_model">(threading::seq, &render::update_instance_object_model, this);
	state.declare<"render::release_renderable_instances">(threading::seq, &render::release_renderable_instances, this);
	state.declare<"render::tick_draws">(threading::seq, &render::tick_draws, this);
}

void render::update_instance_data(
  psl::ecs::info_t& info,
  psl::ecs::pack_indirect_partial_t<const renderable,
									const transform,
									psl::ecs::filter<dynamic_tag, transform_instance_data_tag>,
									psl::ecs::order_by<renderer_sort, renderable>> pack) {
	if(pack.empty()) {
		return;
	}
	psl::UID lastGeometryUID {};
	renderable const* lastRenderable = nullptr;
	psl::array<std::array<float, 16>> instanceData {};
	instanceData.reserve(pack.size());
	psl::array<std::uint32_t> instanceIDs {};
	instanceIDs.reserve(pack.size());
	for(auto [r, t] : pack) {
		if(!r.bundle || !r.geometry) {
			continue;
		}
		if((lastRenderable == nullptr || lastRenderable->bundle.uid() != r.bundle.uid()) ||
		   lastGeometryUID != r.geometry.uid()) {
			if(lastRenderable) {
				auto scoped_lock = std::scoped_lock(m_Mutex);
				auto bundle		 = lastRenderable->bundle;
				bundle->set(lastRenderable->geometry, instanceIDs, "INSTANCE_DATA", std::move(instanceData));
			}
			lastRenderable	= &r;
			lastGeometryUID = r.geometry.uid();
			instanceIDs.clear();
			instanceData.clear();
		}
		auto& inst = instanceData.emplace_back();
		inst[0]	   = t.position[0];
		inst[1]	   = t.position[1];
		inst[2]	   = t.position[2];
		inst[3]	   = 0.0f;	  // padding for vec4 alignment
		inst[4]	   = t.rotation[0];
		inst[5]	   = t.rotation[1];
		inst[6]	   = t.rotation[2];
		inst[7]	   = t.rotation[3];
		inst[8]	   = t.scale[0];
		inst[9]	   = t.scale[1];
		inst[10]   = t.scale[2];
		inst[11]   = 1.0f;	  // padding for vec4 alignment

		instanceIDs.push_back(r.instance_id);
	}
	if(lastRenderable) {
		auto scoped_lock = std::scoped_lock(m_Mutex);
		auto bundle		 = lastRenderable->bundle;
		bundle->set(lastRenderable->geometry, instanceIDs, "INSTANCE_DATA", std::move(instanceData));
	}
}

void render::update_instance_object_model(
  psl::ecs::info_t& info,
  psl::ecs::pack_indirect_partial_t<const renderable,
									const transform,
									psl::ecs::filter<dynamic_tag, transform_instance_object_model_tag>,
									psl::ecs::order_by<renderer_sort, renderable>> pack) {
	if(pack.empty()) {
		return;
	}
	psl::UID lastGeometryUID {};
	renderable const* lastRenderable = nullptr;

	psl::array<psl::mat4x4> modelMats {};
	modelMats.reserve(pack.size());
	psl::array<std::uint32_t> instanceIDs {};
	instanceIDs.reserve(pack.size());
	for(auto [r, t] : pack) {
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
			instanceIDs.clear();
			modelMats.clear();
		}
		const psl::mat4x4 translationMat = psl::math::translate(t.position);
		const psl::mat4x4 rotationMat	 = psl::math::to_matrix(t.rotation);
		const psl::mat4x4 scaleMat		 = psl::math::scale(t.scale);
		const auto modelMat				 = translationMat * rotationMat * scaleMat;
		modelMats.emplace_back(modelMat);

		instanceIDs.push_back(r.instance_id);
	}
	if(lastRenderable) {
		auto scoped_lock = std::scoped_lock(m_Mutex);
		auto bundle		 = lastRenderable->bundle;
		bundle->set(
		  lastRenderable->geometry, instanceIDs, core::gfx::constants::INSTANCE_MODELMATRIX, std::move(modelMats));
	}
}

void render::release_renderable_instances(
  info_t& info,
  pack_indirect_partial_t<const renderable, on_remove<renderable>, psl::ecs::order_by<renderer_sort, renderable>>
	pack) {
	if(pack.empty()) {
		return;
	}
	// now for each removed renderable, remove the instance ids from the bundle if they exist.
	auto* render_it = &pack.get<renderable const>()[0];

	psl::array<std::uint32_t> instanceIDs {};
	std::unordered_map<psl::UID, core::resource::handle<core::gfx::bundle>> seenBundles;

	for(auto [render] : pack) {
		auto bundleHandle = render.bundle;
		if(render.instance_id != std::numeric_limits<core::gfx::instancing_size_type>::max()) {
			instanceIDs.push_back(render.instance_id);
		}
		if(bundleHandle.uid() != render_it->bundle.uid() || render_it->geometry.uid() != render.geometry.uid()) {
			{
				auto lck = std::scoped_lock(m_Mutex);
				bundleHandle->release(render.geometry, instanceIDs);
			}
			if(seenBundles.find(bundleHandle.uid()) == seenBundles.end()) {
				seenBundles.insert({bundleHandle.uid(), bundleHandle});
			}
			render_it = &render;
			instanceIDs.clear();
		}
	}

	{
		{
			auto scoped_lock = std::scoped_lock(m_Mutex);
			auto bundle		 = render_it->bundle;
			bundle->release(render_it->geometry, instanceIDs);
		}
		if(seenBundles.find(render_it->bundle.uid()) == seenBundles.end()) {
			seenBundles.insert({render_it->bundle.uid(), render_it->bundle});
		}
	}

	auto scoped_lock = std::scoped_lock(m_Mutex);
	for(auto& [uid, bundle] : seenBundles) {
		bundle->apply();
	}
}


void render::tick_draws(info_t& info,
						pack_indirect_full_t<const renderable, on_add<renderable>> renderables,
						pack_indirect_full_t<const renderable, on_remove<renderable>> broken_renderables) {
	if(!renderables.size() && !broken_renderables.size())
		return;
	m_Pass->dirty(true);
	m_Pass->clear();

	// for each RenderRange, create a drawgroup. assign all bundles to that group
	for(auto renderRange : m_RenderRanges) {
		auto& default_layer = m_DrawGroup.layer("default", renderRange.first, renderRange.second - renderRange.first);
		for(auto [renderable] : renderables) {
			if(renderable.bundle)
				m_DrawGroup.add(default_layer, renderable.bundle).add(renderable.geometry);
		}

		for(auto [renderable] : broken_renderables) {
			if(!renderable.bundle)
				continue;
			if(auto dCall = m_DrawGroup.get(default_layer, renderable.bundle)) {
				dCall.value().get().remove(renderable.geometry.operator const psl::UID&());
			}
		}
	}
	m_Pass->add(m_DrawGroup);
}

void render::add_render_range(uint32_t begin, uint32_t end) {
	m_RenderRanges.emplace_back(std::make_pair(begin, end));
}

void render::remove_render_range(uint32_t begin, uint32_t end) {
	throw std::runtime_error("not implemented");
}
