#include "stdafx.h"
#include "gfx/pipeline_cache.h"
#include "vk/context.h"
#include "data/material.h"
#include "vk/framebuffer.h"
#include "vk/swapchain.h"
#include "vk/pipeline.h"

using namespace psl;
using namespace core::gfx;
using namespace core::resource;

pipeline_cache::pipeline_cache(const UID& uid, core::resource::cache& cache,
							   core::resource::handle<core::gfx::context> context)
	: m_Context(context), m_Cache(cache)
{
	::vk::PipelineCacheCreateInfo pcci;
	if(!utility::vulkan::check(m_Context->device().createPipelineCache(&pcci, nullptr, &m_PipelineCache)))
	{
		core::gfx::log->error("could not create a gfx::pipeline_cache");
	}
}

pipeline_cache::~pipeline_cache() { m_Context->device().destroyPipelineCache(m_PipelineCache); }

// todo: actually generate a hash for these items
core::resource::handle<core::gfx::pipeline> pipeline_cache::get(const psl::UID& uid, handle<core::data::material> data, core::resource::handle<framebuffer> framebuffer)
{
	pipeline_key key(uid, data, framebuffer->render_pass());
	if(auto it = m_Pipelines.find(key); it != std::end(m_Pipelines))
	{
		return it->second;
	}

	auto pipelineHandle = create<pipeline>(m_Cache);
	pipelineHandle.load(m_Context, data, m_PipelineCache, framebuffer->render_pass(), (uint32_t)framebuffer->attachments().size());
	m_Pipelines[key] = pipelineHandle;

	return pipelineHandle;
}

core::resource::handle<core::gfx::pipeline> pipeline_cache::get(const psl::UID& uid, handle<core::data::material> data, core::resource::handle<swapchain> swapchain)
{
	pipeline_key key(uid, data, swapchain->renderpass());
	if(auto it = m_Pipelines.find(key); it != std::end(m_Pipelines))
	{
		return it->second;
	}

	auto pipelineHandle = create<pipeline>(m_Cache);
	pipelineHandle.load(m_Context, data, m_PipelineCache, swapchain->renderpass(), 1);
	m_Pipelines[key] = pipelineHandle;

	return pipelineHandle;
}

std::vector<std::pair<vk::DescriptorType, uint32_t>> fill_in_descriptors(core::resource::handle<core::data::material> data, vk::RenderPass pass)
{
	std::vector<std::pair<vk::DescriptorType, uint32_t>> descriptors;
	for(const auto& stage : data->stages())
	{
		for(const auto& shader_descriptor : stage.bindings())
		{
			descriptors.emplace_back(std::make_pair(shader_descriptor.descriptor(), shader_descriptor.binding_slot()));
		}
	}
	return descriptors;
}

pipeline_key::pipeline_key(const psl::UID& uid, core::resource::handle<core::data::material> data, vk::RenderPass pass)	:	 uid(uid), renderPass(pass), descriptors(fill_in_descriptors(data, pass))
{
	
}