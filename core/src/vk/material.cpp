#include "vk/material.h"
#include "data/buffer.h"
#include "data/material.h"
#include "logging.h"
#include "meta/shader.h"
#include "vk/buffer.h"
#include "vk/context.h"
#include "vk/conversion.h"
#include "vk/framebuffer.h"
#include "vk/pipeline.h"
#include "vk/pipeline_cache.h"
#include "vk/sampler.h"
#include "vk/shader.h"
#include "vk/swapchain.h"
#include "vk/texture.h"

#include "gfx/buffer.h"

using namespace psl;
using namespace core::ivk;
using namespace core::resource;
using namespace core;

material::material(core::resource::cache& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   handle<core::ivk::context> context,
				   handle<core::data::material> data,
				   core::resource::handle<core::ivk::pipeline_cache> pipeline_cache,
				   core::resource::handle<core::ivk::buffer> materialBuffer) :
	m_UID(metaData.uid),
	m_Context(context), m_PipelineCache(pipeline_cache), m_Data(data), m_MaterialBuffer(materialBuffer)
{
	PROFILE_SCOPE(core::profiler)
	const auto& ID = m_UID;
	m_IsValid	   = false;


	for(const auto& stage : m_Data->stages())
	{
		auto shader_handle = cache.find<core::ivk::shader>(stage.shader());
		if(!shader_handle)
		{
			core::gfx::log->warn("ivk::material [{0}] uses a shader [{1}] that cannot be found in the resource cache.",
								 utility::to_string(ID),
								 utility::to_string(stage.shader()));


			core::gfx::log->info("trying to load shader [{0}].", utility::to_string(stage.shader()));
			shader_handle = cache.instantiate<core::ivk::shader>(stage.shader(), context);
			if(!shader_handle)
			{
				core::ivk::log->error("failed to load shader [{0}]", utility::to_string(stage.shader()));
				return;
			}
		}
		m_Shaders.push_back(shader_handle);

		// now we validate the shader, and store all the bound resource handles
		for(const auto& binding : stage.bindings())
		{
			switch(binding.descriptor())
			{
			case core::gfx::binding_type::combined_image_sampler:
			{
				if(auto sampler_handle = cache.find<core::ivk::sampler>(binding.sampler()); sampler_handle)
				{
					m_Samplers.push_back(std::make_pair(binding.binding_slot(), sampler_handle));
				}
				else
				{
					// todo: add error sampler as fallback when no sampler can be found
					core::gfx::log->error(
					  "ivk::material [{0}] uses a sampler [{1}] in shader [{2}] that cannot be found in the resource "
					  "cache.",
					  utility::to_string(ID),
					  utility::to_string(binding.sampler()),
					  utility::to_string(stage.shader()));
					return;
				}
				if(auto texture_handle = cache.find<core::ivk::texture>(binding.texture()); texture_handle)
				{
					m_Textures.push_back(std::make_pair(binding.binding_slot(), texture_handle));
				}
				else
				{
					// todo: add error texture as fallback when no texture can be found
					core::gfx::log->error(
					  "ivk::material [{0}] uses a texture [{1}] in shader [{2}] that cannot be found in the resource "
					  "cache.",
					  utility::to_string(ID),
					  utility::to_string(binding.texture()),
					  utility::to_string(stage.shader()));
					return;
				}
			}
			break;
			case core::gfx::binding_type::uniform_buffer_dynamic:
			case core::gfx::binding_type::storage_buffer_dynamic:
				m_DynamicOffsets.emplace_back(0);
				m_DynamicOffsetsIndices.emplace_back(binding.binding_slot());
			case core::gfx::binding_type::uniform_buffer:
			case core::gfx::binding_type::storage_buffer:
			{
				if(auto buffer_handle = cache.find<core::gfx::shader_buffer_binding>(binding.buffer());
				   buffer_handle && buffer_handle->buffer.state() == core::resource::state::loaded)
				{
					vk::BufferUsageFlagBits usage =
					  (binding.descriptor() == core::gfx::binding_type::uniform_buffer ||
					   binding.descriptor() == core::gfx::binding_type::uniform_buffer_dynamic)
						? vk::BufferUsageFlagBits::eUniformBuffer
						: vk::BufferUsageFlagBits::eStorageBuffer;
					if(!(core::gfx::conversion::to_vk(buffer_handle->buffer->data().usage()) & usage))
					{
						core::gfx::log->error(
						  "ivk::material [{0}] declares resource of the type [{1}], but we detected a resource of "
						  "the type [{2}] instead in shader [{3}]",
						  utility::to_string(ID),
						  vk::to_string(gfx::conversion::to_vk(binding.descriptor())),
						  vk::to_string(core::gfx::conversion::to_vk(buffer_handle->buffer->data().usage())),
						  utility::to_string(stage.shader()));
						return;
					}
				}
				else
				{
					core::gfx::log->error(
					  "ivk::material [{0}] uses a buffer [{1}] in shader [{2}] that cannot be found in the resource "
					  "cache.",
					  utility::to_string(ID),
					  utility::to_string(binding.buffer()),
					  utility::to_string(stage.shader()));
					return;
				}
			}
			break;

			default:
				throw std::runtime_error("This should not be reached");
				return;
			}
		}
	}

	std::sort(std::begin(m_DynamicOffsetsIndices), std::end(m_DynamicOffsetsIndices));

	m_IsValid = true;
};

material::~material() {}

core::resource::handle<core::data::material> material::data() const { return m_Data; }
const std::vector<core::resource::handle<core::ivk::shader>>& material::shaders() const { return m_Shaders; }
const std::vector<std::pair<uint32_t, core::resource::handle<core::ivk::texture>>>& material::textures() const
{
	return m_Textures;
}
const std::vector<std::pair<uint32_t, core::resource::handle<core::ivk::sampler>>>& material::samplers() const
{
	return m_Samplers;
}

core::resource::handle<pipeline> material::get(core::resource::handle<framebuffer> framebuffer)
{
	PROFILE_SCOPE(core::profiler)
	if(auto it = m_Pipeline.find(framebuffer); it == std::end(m_Pipeline))
	{
		m_Pipeline[framebuffer] = m_PipelineCache->get(m_UID, m_Data, framebuffer);
		return m_Pipeline[framebuffer];
	}
	else
	{
		return it->second;
	}
}

core::resource::handle<pipeline> material::get(core::resource::handle<swapchain> swapchain)
{
	PROFILE_SCOPE(core::profiler)
	if(auto it = m_Pipeline.find(swapchain); it == std::end(m_Pipeline))
	{
		m_Pipeline[swapchain] = m_PipelineCache->get(m_UID, m_Data, swapchain);
		return m_Pipeline[swapchain];
	}
	else
	{
		return it->second;
	}
}

bool material::bind_pipeline(vk::CommandBuffer cmdBuffer,
							 core::resource::handle<framebuffer> framebuffer,
							 uint32_t drawIndex)
{
	PROFILE_SCOPE(core::profiler)
	m_Bound = get(framebuffer);
	if(m_Bound->has_pushconstants())
	{
		cmdBuffer.pushConstants(m_Bound->vkLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), &drawIndex);
	}

	if(m_MaterialBufferRange.range().size() > 0)
	{
		m_Bound->update(
		  m_MaterialBufferBinding, m_MaterialBufferRange.range().begin, m_MaterialBufferRange.range().size());
	}

	if(!m_Bound->is_complete())
	{
		core::ivk::log->error(
		  "tried to bind an incomplete or invalid pipeline, please inspect the logs around material {}",
		  m_UID.to_string());
		return false;
	}

	return m_Bound->bind(cmdBuffer, m_DynamicOffsets);
}

bool material::bind_pipeline(vk::CommandBuffer cmdBuffer,
							 core::resource::handle<swapchain> swapchain,
							 uint32_t drawIndex)
{
	PROFILE_SCOPE(core::profiler)
	m_Bound = get(swapchain);
	if(m_Bound->has_pushconstants())
	{
		cmdBuffer.pushConstants(m_Bound->vkLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), &drawIndex);
	}

	if(!m_Bound->is_complete())
	{
		core::ivk::log->error(
		  "tried to bind an incomplete or invalid pipeline, please inspect the logs around material {}",
		  m_UID.to_string());
		return false;
	}


	return m_Bound->bind(cmdBuffer, m_DynamicOffsets);
}

void material::bind_material_instance_data(core::resource::handle<core::ivk::buffer> buffer, memory::segment segment)
{
	// assert(segment.range().size() <= m_MaterialBuffer->data()->size());
	m_MaterialBufferRange = segment;
	// m_MaterialBuffer->copy_from(buffer.value(), {vk::BufferCopy{segment.range().begin, 0, segment.range().size()}});
}
bool material::bind_instance_data(uint32_t binding, uint32_t offset)
{
	auto it = std::find(std::begin(m_DynamicOffsetsIndices), std::end(m_DynamicOffsetsIndices), binding);
	if(it == std::end(m_DynamicOffsetsIndices))
	{
		core::ivk::log->error("the requested binding slot {} was not found in the material", binding);
		return false;
	}

	m_DynamicOffsets[std::distance(std::begin(m_DynamicOffsetsIndices), it)] = offset;
}