#include "stdafx.h"
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

using namespace core::gfx;
using namespace core::resource;
using namespace core;

material::material(resource_dependency packet, handle<core::gfx::context> context, handle<core::data::material> data,
				   core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
				   core::resource::handle<core::gfx::buffer> materialBuffer,
				   core::resource::handle<core::gfx::buffer> instanceBuffer)
	: m_Context(context), m_PipelineCache(pipeline_cache), m_Data(data), m_MaterialBuffer(materialBuffer),
	  m_InstanceBuffer(instanceBuffer)
{
	const auto& ID = packet.get<UID>();
	auto& cache	= packet.get<core::resource::cache>();
	m_IsValid	  = false;

	for(const auto& stage : m_Data->stages())
	{
		// todo: decide if shaders should be loaded when materials get constructed or not
		auto shader_handle = cache.find<core::gfx::shader>(stage.shader());
		if(!shader_handle)
		{
			core::gfx::log->warn("gfx::material [{0}] uses a shader [{1}] that cannot be found in the resource cache.", utility::to_string(ID), utility::to_string(stage.shader()));

			
			core::gfx::log->info("trying to load shader [{0}].", utility::to_string(stage.shader()));
			shader_handle = create<core::gfx::shader>(cache, stage.shader());
			if(!shader_handle.load(context))
				return;
		}
		m_Shaders.push_back(shader_handle);

		if(stage.shader_stage() == vk::ShaderStageFlagBits::eVertex)
		{
			std::vector<instance_element> elements;
			for(const auto& vBinding : shader_handle->meta()->instance_bindings())
			{
				instance_element& iData = elements.emplace_back();
				iData.slot = vBinding.binding_slot();
				iData.size_of_element = vBinding.size();
				iData.name = vBinding.buffer();
			}
			m_InstanceData = instance_data(std::move(elements), 1024*1024);
		}
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
					core::gfx::log->error("gfx::material [{0}] uses a sampler [{1}] in shader [{2}] that cannot be found in the resource cache.", utility::to_string(ID), utility::to_string(binding.sampler()), utility::to_string(stage.shader()));
					return;
				}
				if(auto texture_handle = cache.find<core::gfx::texture>(binding.texture()); texture_handle)
				{
					m_Textures.push_back(std::make_pair(binding.binding_slot(), texture_handle));
				}
				else
				{
					core::gfx::log->error("gfx::material [{0}] uses a texture [{1}] in shader [{2}] that cannot be found in the resource cache.", utility::to_string(ID), utility::to_string(binding.texture()), utility::to_string(stage.shader()));
					return;
				}
			}
			break;
			case vk::DescriptorType::eUniformBuffer:
			case vk::DescriptorType::eStorageBuffer:
			{

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
						core::gfx::log->error("gfx::material [{0}] declares resource of the type [{1}], but we detected a resource of the type [{2}] instead in shader [{3}]", utility::to_string(ID), vk::to_string(binding.descriptor()), vk::to_string(buffer_handle->data()->usage()), utility::to_string(stage.shader()));
						return;
					}
				}
				else
				{
					core::gfx::log->error("gfx::material [{0}] uses a buffer [{1}] in shader [{2}] that cannot be found in the resource cache.", utility::to_string(ID), utility::to_string(binding.buffer()), utility::to_string(stage.shader()));
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

material::~material()
{
	m_InstanceData.remove_all(m_InstanceBuffer);
}

core::resource::handle<core::data::material> material::data() const { return m_Data; }
const std::vector<core::resource::handle<core::gfx::shader>>& material::shaders() const { return m_Shaders; }
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
	if(auto it = m_Pipeline.find(framebuffer.ID()); it == std::end(m_Pipeline))
	{
		m_Pipeline[framebuffer.ID()] = m_PipelineCache->get(m_Data, framebuffer);
		return m_Pipeline[framebuffer.ID()];
	}
	else
	{
		return it->second;
	}
}

core::resource::handle<pipeline> material::get(core::resource::handle<swapchain> swapchain)
{
	if(auto it = m_Pipeline.find(swapchain.ID()); it == std::end(m_Pipeline))
	{
		m_Pipeline[swapchain.ID()] = m_PipelineCache->get(m_Data, swapchain);
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
	m_Bound = get(framebuffer);
	if(m_Bound->has_pushconstants())
	{
		cmdBuffer.pushConstants(m_Bound->vkLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), &drawIndex);
	}

	// Bind the rendering pipeline (including the shaders)
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Bound->vkPipeline());

	// Bind descriptor sets describing shader binding points
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_Bound->vkLayout(), 0, 1,
								 m_Bound->vkDescriptorSet(), 0,
								 nullptr);

	return true;
}

bool material::bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<swapchain> swapchain,
							 uint32_t drawIndex)
{
	m_Bound = get(swapchain);
	if(m_Bound->has_pushconstants())
	{
		cmdBuffer.pushConstants(m_Bound->vkLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), &drawIndex);
	}

	// Bind the rendering pipeline (including the shaders)
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Bound->vkPipeline());

	// Bind descriptor sets describing shader binding points
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_Bound->vkLayout(), 0, 1,
								 m_Bound->vkDescriptorSet(), 0,
								 nullptr);

	// todo: material data is written here.

	return true;
}

bool material::bind_geometry(vk::CommandBuffer cmdBuffer, const core::resource::handle<geometry> geometry)
{
	if(auto iDataIt = m_InstanceData.instance(geometry.ID()); iDataIt)
	{
		auto& iData = iDataIt.value().get();		

		for(const auto& b : iData.segments)
		{
			cmdBuffer.bindVertexBuffers(b.slot, 1, &m_InstanceBuffer->gpu_buffer(), &b.segment.range().begin);
		}

		return true;
	}
	return false;
}


uint32_t material::instances(const core::resource::handle<core::gfx::geometry> geometry) const
{ 
	return m_InstanceData.has_data()? m_InstanceData.size(geometry.ID()) : 1u;
}


bool material::set(const core::resource::tag<core::gfx::geometry>& geometry, uint32_t id, uint32_t binding, const void* data,
		 size_t size)
{
	if(auto it = m_InstanceData.instance(geometry); it)
	{
		auto& instance_obj = it.value().get();
		if(!instance_obj.id_generator.IsID(id))
			return false;

		for(const auto& element : instance_obj.segments)
		{
			if(element.slot == binding)
			{
				m_InstanceBuffer->commit( { core::gfx::buffer::commit_instruction{(void*)data, size, element.segment, memory::range{element.size_of_element * id, element.size_of_element * (id + 1) }}});
				return true;
			}
		}
	}
	return false;
}


bool material::set(const core::resource::tag<core::gfx::geometry>& geometry, uint32_t id_first, uint32_t binding, const void* data,
		 size_t size, size_t count)
{
	auto opt = m_InstanceData.instance(geometry);
	if(!opt)
		return false;

	auto& instance_obj = opt.value().get();
	
	for(const auto& element : instance_obj.segments)
	{
		if(element.slot == binding)
		{
			m_InstanceBuffer->commit({core::gfx::buffer::commit_instruction{(void*)data, size * count, element.segment, memory::range{element.size_of_element * id_first, element.size_of_element * (id_first + count) }}});
			return true;
		}
	}
	return false;
}


std::optional<uint32_t> material::instantiate(const core::resource::tag<core::gfx::geometry>& geometry)
{
	return m_InstanceData.add(m_InstanceBuffer, geometry);
}

bool material::release(const core::resource::tag<core::gfx::geometry>& geometry, uint32_t id)
{
	return m_InstanceData.remove(m_InstanceBuffer, geometry, id);
}
bool material::release_all()
{
	return m_InstanceData.remove_all(m_InstanceBuffer);
}

std::optional<uint32_t> material::instance_data::add(core::resource::handle<core::gfx::buffer> buffer, const UID& uid)
{
	if(!has_data()) return std::nullopt;
	auto it = m_Instance.find(uid);
	if(it == std::end(m_Instance))
	{
		it = m_Instance.emplace(uid, instance_object{m_Capacity, elements.size()}).first;
		for(auto elementIt : elements)
		{
			auto& data = it->second.segments.emplace_back();
			data.slot = elementIt.slot;
			data.size_of_element = elementIt.size_of_element;
			data.segment = buffer->reserve(elementIt.size_of_element * m_Capacity).value();
		}
	}

	if(it->second.size == m_Capacity) return std::nullopt;

	if(auto id = it->second.id_generator.CreateID(); id.first)
	{
		++it->second.size;
		return id.second;
	}
	return std::nullopt;
}

bool material::instance_data::remove(core::resource::handle<core::gfx::buffer> buffer, const UID& uid, uint32_t id)
{
	if(auto it = m_Instance.find(uid); it != std::end(m_Instance) && it->second.id_generator.DestroyID(id))
	{
		--it->second.size;

		if(it->second.size == 0)
		{
			for(auto& segment : it->second.segments)
			{
				buffer->deallocate(segment.segment);
			}
			m_Instance.erase(it);
		}
		return true;
	}
	return false;
}

material::optional_ref<const material::instance_element> material::instance_data::has_element(psl::string_view name) const noexcept
{
	if(auto it = std::find_if(std::begin(elements), std::end(elements),
							  [&name](const instance_element& element) { return element.name == name; });
	   it != std::end(elements))
	{
		return *it;
	}
	return std::nullopt;
}
material::optional_ref<const material::instance_element> material::instance_data::has_element(uint32_t slot) const noexcept
{
	if(auto it = std::find_if(std::begin(elements), std::end(elements),
							  [&slot](const instance_element& element) { return element.slot == slot; });
	   it != std::end(elements))
	{
		return *it;
	}
	return std::nullopt;
}

material::optional_ref<material::instance_object> material::instance_data::instance(const UID& uid) noexcept
{
	auto it = m_Instance.find(uid);
	if(it != std::end(m_Instance))
		return it->second;
	return std::nullopt;
}

uint32_t material::instance_data::size(const UID& uid) const noexcept
{
	if(auto it = m_Instance.find(uid); it != std::end(m_Instance))
	{
		return it->second.size;
	}
	return 0;
}


bool material::instance_data::remove_all(core::resource::handle<core::gfx::buffer> buffer)
{
	for(auto& instance : m_Instance)
	{
		for(auto& segment : instance.second.segments)
		{
			buffer->deallocate(segment.segment);
		}
	}
	m_Instance.clear();
	return true;
}
bool material::instance_data::remove_all(core::resource::handle<core::gfx::buffer> buffer, const UID& uid)
{
	if(auto it = m_Instance.find(uid); it != std::end(m_Instance))
	{
		for(auto& segment : it->second.segments)
		{
			buffer->deallocate(segment.segment);
		}
		m_Instance.erase(it);
		return true;
	}
	return false;
}