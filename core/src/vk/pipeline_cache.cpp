#include "core/vk/pipeline_cache.hpp"
#include "core/data/material.hpp"
#include "core/logging.hpp"
#include "core/vk/context.hpp"
#include "core/vk/conversion.hpp"
#include "core/vk/framebuffer.hpp"
#include "core/vk/pipeline.hpp"
#include "core/vk/swapchain.hpp"
#include "psl/meta.hpp"

using namespace psl;
using namespace core::gfx;
using namespace core::ivk;
using namespace core::resource;

pipeline_cache::pipeline_cache(core::resource::cache_t& cache,
							   const core::resource::metadata& metaData,
							   psl::meta::file* metaFile,
							   core::resource::handle<core::ivk::context> context)
	: m_Context(context), m_Cache(cache) {
	::vk::PipelineCacheCreateInfo pcci;
	if(!utility::vulkan::check(m_Context->device().createPipelineCache(&pcci, nullptr, &m_PipelineCache))) {
		core::gfx::log->error("could not create a ivk::pipeline_cache");
	}
}

pipeline_cache::~pipeline_cache() {
	m_Context->device().destroyPipelineCache(m_PipelineCache);
}

// todo: actually generate a hash for these items
core::resource::handle<core::ivk::pipeline> pipeline_cache::get(const psl::UID& uid,
																handle<core::data::material_t> data,
																core::resource::handle<framebuffer_t> framebuffer) {
	pipeline_key key(uid, data, framebuffer->render_pass());
	if(auto it = m_Pipelines.find(key); it != std::end(m_Pipelines)) {
		return it->second;
	}

	auto pipelineHandle = m_Cache.create<pipeline>(
	  m_Context, data, m_PipelineCache, framebuffer->render_pass(), (uint32_t)framebuffer->color_attachments().size());
	m_Pipelines[key] = pipelineHandle;

	return pipelineHandle;
}

core::resource::handle<core::ivk::pipeline> pipeline_cache::get(const psl::UID& uid,
																handle<core::data::material_t> data,
																core::resource::handle<swapchain> swapchain) {
	pipeline_key key(uid, data, swapchain->renderpass());
	if(auto it = m_Pipelines.find(key); it != std::end(m_Pipelines)) {
		return it->second;
	}

	auto pipelineHandle = m_Cache.create<pipeline>(m_Context, data, m_PipelineCache, swapchain->renderpass(), 1);
	m_Pipelines[key]	= pipelineHandle;

	return pipelineHandle;
}

std::vector<std::pair<vk::DescriptorType, uint32_t>>
fill_in_descriptors(core::resource::handle<core::data::material_t> data, vk::RenderPass pass) {
	std::vector<std::pair<vk::DescriptorType, uint32_t>> descriptors;
	for(const auto& stage : data->stages()) {
		for(const auto& shader_descriptor : stage.bindings()) {
			descriptors.emplace_back(
			  std::make_pair(conversion::to_vk(shader_descriptor.descriptor()), shader_descriptor.binding_slot()));
		}
	}
	return descriptors;
}

pipeline_key::pipeline_key(const psl::UID& uid,
						   core::resource::handle<core::data::material_t> data,
						   vk::RenderPass pass)
	: uid(uid), renderPass(pass), descriptors(fill_in_descriptors(data, pass)) {}
