#include "logging.h"
#include "vk/context.h"
#include "gfx/material.h"
#include "data/material.h"
#include "gfx/pipeline_cache.h"
#include "meta/shader.h"
#include "vk/shader.h"
#include "vk/buffer.h"
#include "vk/texture.h"
#include "vk/sampler.h"
#include "data/buffer.h"
#include "vk/swapchain.h"
#include "vk/framebuffer.h"
#include "vk/pipeline.h"

using namespace psl;
using namespace core::gfx;
using namespace core::ivk;
using namespace core::resource;
using namespace core;

material::material(psl::UID uid, core::resource::cache& cache, handle<core::ivk::context> context,
				   handle<core::data::material> data,
				   core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
				   core::resource::handle<core::gfx::buffer> materialBuffer)
	: m_UID(std::move(uid)), m_Context(context), m_PipelineCache(pipeline_cache), m_Data(data),
	  m_MaterialBuffer(materialBuffer)
{
	PROFILE_SCOPE(core::profiler)
	const auto& ID = m_UID;
	m_IsValid	  = false;

	for(const auto& stage : m_Data->stages())
	{
		// todo: decide if shaders should be loaded when materials get constructed or not
		auto shader_handle = cache.find<core::ivk::shader>(stage.shader());
		if(!shader_handle)
		{
			core::gfx::log->warn("gfx::material [{0}] uses a shader [{1}] that cannot be found in the resource cache.",
								 utility::to_string(ID), utility::to_string(stage.shader()));


			core::gfx::log->info("trying to load shader [{0}].", utility::to_string(stage.shader()));
			shader_handle = create<core::ivk::shader>(cache, stage.shader());
			if(!shader_handle.load(context)) return;
		}
		m_Shaders.push_back(shader_handle);

		// now we validate the shader, and store all the bound resource handles
		for(const auto& binding : stage.bindings())
		{
			switch(binding.descriptor())
			{
			case vk::DescriptorType::eCombinedImageSampler:
			{
				if(auto sampler_handle = cache.find<core::gfx::sampler>(binding.sampler()); sampler_handle)
				{
					m_Samplers.push_back(std::make_pair(binding.binding_slot(), sampler_handle));
				}
				else
				{
					core::gfx::log->error(
						"gfx::material [{0}] uses a sampler [{1}] in shader [{2}] that cannot be found in the resource "
						"cache.",
						utility::to_string(ID), utility::to_string(binding.sampler()),
						utility::to_string(stage.shader()));
					return;
				}
				if(auto texture_handle = cache.find<core::gfx::texture>(binding.texture()); texture_handle)
				{
					m_Textures.push_back(std::make_pair(binding.binding_slot(), texture_handle));
				}
				else
				{
					core::gfx::log->error(
						"gfx::material [{0}] uses a texture [{1}] in shader [{2}] that cannot be found in the resource "
						"cache.",
						utility::to_string(ID), utility::to_string(binding.texture()),
						utility::to_string(stage.shader()));
					return;
				}
			}
			break;
			case vk::DescriptorType::eUniformBuffer:
			case vk::DescriptorType::eStorageBuffer:
			{
				//if(binding.buffer() == "MATERIAL_DATA") continue;
				if(auto buffer_handle = cache.find<core::gfx::buffer>(binding.buffer());
				   buffer_handle && buffer_handle.resource_state() == core::resource::state::LOADED)
				{
					vk::BufferUsageFlagBits usage = (binding.descriptor() == vk::DescriptorType::eUniformBuffer)
														? vk::BufferUsageFlagBits::eUniformBuffer
														: vk::BufferUsageFlagBits::eStorageBuffer;
					if(buffer_handle->data()->usage() & usage)
					{
						m_Buffers.push_back(std::make_pair(binding.binding_slot(), buffer_handle));
					}
					else
					{
						core::gfx::log->error(
							"gfx::material [{0}] declares resource of the type [{1}], but we detected a resource of "
							"the type [{2}] instead in shader [{3}]",
							utility::to_string(ID), vk::to_string(binding.descriptor()),
							vk::to_string(buffer_handle->data()->usage()), utility::to_string(stage.shader()));
						return;
					}
				}
				else
				{
					core::gfx::log->error(
						"gfx::material [{0}] uses a buffer [{1}] in shader [{2}] that cannot be found in the resource "
						"cache.",
						utility::to_string(ID), utility::to_string(binding.buffer()),
						utility::to_string(stage.shader()));
					return;
				}
			}
			break;

			default: throw new std::runtime_error("This should not be reached"); return;
			}
		}
	}

	m_IsValid = true;
};

material::~material() {}

core::resource::handle<core::data::material> material::data() const { return m_Data; }
const std::vector<core::resource::handle<core::ivk::shader>>& material::shaders() const { return m_Shaders; }
const std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::texture>>>& material::textures() const
{
	return m_Textures;
}
const std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::sampler>>>& material::samplers() const
{
	return m_Samplers;
}
const std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::buffer>>>& material::buffers() const
{
	return m_Buffers;
}

// todo
core::resource::handle<pipeline> material::get(core::resource::handle<framebuffer> framebuffer)
{
	PROFILE_SCOPE(core::profiler)
	if(auto it = m_Pipeline.find(framebuffer.ID()); it == std::end(m_Pipeline))
	{
		m_Pipeline[framebuffer.ID()] = m_PipelineCache->get(m_UID, m_Data, framebuffer);
		return m_Pipeline[framebuffer.ID()];
	}
	else
	{
		return it->second;
	}
}

core::resource::handle<pipeline> material::get(core::resource::handle<swapchain> swapchain)
{
	PROFILE_SCOPE(core::profiler)
	if(auto it = m_Pipeline.find(swapchain.ID()); it == std::end(m_Pipeline))
	{
		m_Pipeline[swapchain.ID()] = m_PipelineCache->get(m_UID, m_Data, swapchain);
		return m_Pipeline[swapchain.ID()];
	}
	else
	{
		return it->second;
	}
}

bool material::bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<framebuffer> framebuffer,
							 uint32_t drawIndex)
{
	PROFILE_SCOPE(core::profiler)
	m_Bound = get(framebuffer);
	if(m_Bound->has_pushconstants())
	{
		cmdBuffer.pushConstants(m_Bound->vkLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), &drawIndex);
	}

	// Bind the rendering pipeline (including the shaders)
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Bound->vkPipeline());

	// Bind descriptor sets describing shader binding points
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_Bound->vkLayout(), 0, 1,
								 m_Bound->vkDescriptorSet(), 0, nullptr);

	return true;
}

bool material::bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<swapchain> swapchain,
							 uint32_t drawIndex)
{
	PROFILE_SCOPE(core::profiler)
	m_Bound = get(swapchain);
	if(m_Bound->has_pushconstants())
	{
		cmdBuffer.pushConstants(m_Bound->vkLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), &drawIndex);
	}

	// Bind the rendering pipeline (including the shaders)
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Bound->vkPipeline());

	// Bind descriptor sets describing shader binding points
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_Bound->vkLayout(), 0, 1,
								 m_Bound->vkDescriptorSet(), 0, nullptr);

	// todo: material data is written here.

	return true;
}
