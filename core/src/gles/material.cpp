#include "gles/material.h"
#include "data/material.h"
#include "data/buffer.h"
#include "gles/buffer.h"
#include "meta/shader.h"
#include "gles/program.h"
#include "gles/program_cache.h"
#include "gles/sampler.h"
#include "gles/shader.h"
#include "gles/texture.h"

#include "logging.h"

using namespace core::igles;
using namespace core::resource;
namespace data = core::data;

material::material(const psl::UID& uid, cache& cache, handle<data::material> data,
				   core::resource::handle<core::igles::program_cache> program_cache, handle<buffer> matBuffer)
	: m_Data(data)
{
	for(auto& stage : data->stages())
	{
		auto shader_handle = cache.find<core::igles::shader>(stage.shader());
		if(!shader_handle)
		{
			core::igles::log->warn(
				"igles::material [{0}] uses a shader [{1}] that cannot be found in the resource cache.",
				utility::to_string(uid), utility::to_string(stage.shader()));


			core::igles::log->info("trying to load shader [{0}].", utility::to_string(stage.shader()));
			shader_handle = create<core::igles::shader>(cache, stage.shader());
			if(!shader_handle.load()) return;
		}

		m_Shaders.emplace_back(shader_handle);

		// now we validate the shader, and store all the bound resource handles
		for(const auto& binding : stage.bindings())
		{
			switch(binding.descriptor())
			{
			case core::gfx::binding_type::combined_image_sampler:
			{
				if(auto sampler_handle = cache.find<core::igles::sampler>(binding.sampler()); sampler_handle)
				{
					m_Samplers.push_back(std::make_pair(binding.binding_slot(), sampler_handle));
				}
				else
				{
					core::igles::log->error(
						"igles::material [{0}] uses a sampler [{1}] in shader [{2}] that cannot be found in the "
						"resource "
						"cache.",
						utility::to_string(uid), utility::to_string(binding.sampler()),
						utility::to_string(stage.shader()));
					return;
				}
				if(auto texture_handle = cache.find<core::igles::texture>(binding.texture()); texture_handle)
				{
					m_Textures.push_back(std::make_pair(binding.binding_slot(), texture_handle));
				}
				else
				{
					core::igles::log->error(
						"igles::material [{0}] uses a texture [{1}] in shader [{2}] that cannot be found in the "
						"resource "
						"cache.",
						utility::to_string(uid), utility::to_string(binding.texture()),
						utility::to_string(stage.shader()));
					return;
				}
			}
			break;
			case core::gfx::binding_type::uniform_buffer:
			case core::gfx::binding_type::storage_buffer:
			{
				// if(binding.buffer() == "MATERIAL_DATA") continue;
				if(auto buffer_handle = cache.find<core::igles::buffer>(binding.buffer());
				   buffer_handle && buffer_handle.resource_state() == core::resource::state::LOADED)
				{
					vk::BufferUsageFlagBits usage = (binding.descriptor() == core::gfx::binding_type::uniform_buffer)
														? vk::BufferUsageFlagBits::eUniformBuffer
														: vk::BufferUsageFlagBits::eStorageBuffer;
					if(buffer_handle->data().usage() & usage)
					{
						m_Buffers.push_back(std::make_pair(binding.binding_slot(), buffer_handle));
					}
					else
					{
						core::igles::log->error(
							"igles::material [{0}] declares resource of the type [{1}], but we detected a resource of "
							"the type [{2}] instead in shader [{3}]",
							utility::to_string(uid), /*vk::to_string(conversion::to_vk(binding.descriptor())),
							vk::to_string(buffer_handle->data()->usage())*/
							"", "", utility::to_string(stage.shader()));
						return;
					}
				}
				else
				{
					core::igles::log->error(
						"igles::material [{0}] uses a buffer [{1}] in shader [{2}] that cannot be found in the "
						"resource "
						"cache.",
						utility::to_string(uid), utility::to_string(binding.buffer()),
						utility::to_string(stage.shader()));
					return;
				}
			}
			break;

			default: throw new std::runtime_error("This should not be reached"); return;
			}
		}
	}

	m_Program = program_cache->get(uid, data);
}

void material::bind()
{
	if(!m_Program) return;
	glUseProgram(m_Program->id());

	for(auto i = 0; i < m_Textures.size(); ++i)
	{
		auto binding = m_Textures[i].first;

		glActiveTexture(GL_TEXTURE0 + binding);
		glBindTexture(GL_TEXTURE_2D, m_Textures[i].second->id());
		glBindSampler(binding, m_Samplers[i].second->id());
	}

	for(auto& buffer : m_Buffers)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, buffer.first, buffer.second->id());
	}
}

const std::vector<core::resource::handle<core::igles::shader>>& material::shaders() const noexcept { return m_Shaders; }

const core::data::material& material::data() const noexcept { return m_Data; }