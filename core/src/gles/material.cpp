#include "core/gles/material.hpp"
#include "core/data/buffer.hpp"
#include "core/data/material.hpp"
#include "core/gles/buffer.hpp"
#include "core/gles/conversion.hpp"
#include "core/gles/program.hpp"
#include "core/gles/program_cache.hpp"
#include "core/gles/sampler.hpp"
#include "core/gles/shader.hpp"
#include "core/gles/texture.hpp"
#include "core/meta/shader.hpp"

#include "core/gles/igles.hpp"
#include "core/logging.hpp"

#include "core/gfx/buffer.hpp"
#include "core/gfx/details/instance.hpp"

#include "psl/memory/segment.hpp"

using namespace core::igles;
using namespace core::resource;
namespace data = core::data;

material_t::material_t(core::resource::cache_t& cache,
					   const core::resource::metadata& metaData,
					   psl::meta::file* metaFile,
					   handle<data::material_t> data,
					   core::resource::handle<core::igles::program_cache> program_cache)
	: m_Data(data) {
	for(auto& stage : data->stages()) {
		auto shader_handle = cache.find<core::igles::shader>(stage.shader());
		if(!shader_handle) {
			core::igles::log->warn(
			  "igles::material_t [{0}] uses a shader [{1}] that cannot be found in the resource cache.",
			  psl::utility::to_string(metaData.uid),
			  psl::utility::to_string(stage.shader()));


			core::igles::log->info("trying to load shader [{0}].", psl::utility::to_string(stage.shader()));
			shader_handle = cache.instantiate<core::igles::shader>(stage.shader());
			if(!shader_handle) {
				core::igles::log->error("failed to load shader [{0}]", psl::utility::to_string(stage.shader()));
				return;
			}
		}

		m_Shaders.emplace_back(shader_handle);
	}

	m_Program = program_cache->get(metaData.uid, data);
	for(auto& stage : data->stages()) {
		auto meta = cache.library().get<core::meta::shader>(stage.shader()).value_or(nullptr);
		// now we validate the shader, and store all the bound resource handles
		auto index = 0;
		for(const auto& binding : stage.bindings()) {
			switch(binding.descriptor()) {
			case core::gfx::binding_type::combined_image_sampler: {
				auto binding_slot = glGetUniformLocation(m_Program->id(), meta->descriptors()[index].name().data());

				if(auto sampler_handle = cache.find<core::igles::sampler_t>(binding.sampler()); sampler_handle) {
					m_Samplers.push_back(std::make_pair(binding_slot, sampler_handle));
				} else {
					core::igles::log->error(
					  "igles::material_t [{0}] uses a sampler [{1}] in shader [{2}] that cannot be found in the "
					  "resource "
					  "cache.",
					  psl::utility::to_string(metaData.uid),
					  psl::utility::to_string(binding.sampler()),
					  psl::utility::to_string(stage.shader()));
					return;
				}
				if(auto texture_handle = cache.find<core::igles::texture_t>(binding.texture()); texture_handle) {
					m_Textures.push_back(std::make_pair(binding_slot, texture_handle));
				} else {
					core::igles::log->error(
					  "igles::material_t [{0}] uses a texture [{1}] in shader [{2}] that cannot be found in the "
					  "resource "
					  "cache.",
					  psl::utility::to_string(metaData.uid),
					  psl::utility::to_string(binding.texture()),
					  psl::utility::to_string(stage.shader()));
					return;
				}
			} break;
			case core::gfx::binding_type::uniform_buffer_dynamic:
			case core::gfx::binding_type::storage_buffer_dynamic:
			case core::gfx::binding_type::uniform_buffer:
			case core::gfx::binding_type::storage_buffer: {
				auto descriptor = std::find_if(std::begin(meta->descriptors()),
											   std::end(meta->descriptors()),
											   [&binding](const core::meta::shader::descriptor& descriptor) {
												   return descriptor.binding() == binding.binding_slot();
											   });

				if(auto buffer_handle = cache.find<core::gfx::shader_buffer_binding>(binding.buffer());
				   buffer_handle && buffer_handle.state() == core::resource::status::loaded) {
					auto binding_slot = glGetUniformBlockIndex(m_Program->id(), descriptor->name().data());
					glUniformBlockBinding(m_Program->id(), binding_slot, binding.binding_slot());


					auto usage = (binding.descriptor() == core::gfx::binding_type::uniform_buffer ||
								  binding.descriptor() == core::gfx::binding_type::uniform_buffer_dynamic)
								   ? core::gfx::memory_usage::uniform_buffer
								   : core::gfx::memory_usage::storage_buffer;
					if(buffer_handle->buffer->data().usage() & usage) {
						m_Buffers.emplace_back(
						  buffer_binding {buffer_handle->buffer->resource<gfx::graphics_backend::gles>(),
										  binding.binding_slot(),
										  0,
										  static_cast<uint32_t>(descriptor->size())});
					} else {
						core::igles::log->error(
						  "igles::material_t [{0}] declares resource of the type [{1}], but we detected a resource of "
						  "the type [{2}] instead in shader [{3}]",
						  psl::utility::to_string(
							metaData.uid), /*vk::to_string(conversion::to_vk(binding.descriptor())),
						  vk::to_string(buffer_handle->data()->usage())*/
						  "",
						  "",
						  psl::utility::to_string(stage.shader()));
						return;
					}
				} else {
					core::igles::log->error(
					  "igles::material_t [{0}] uses a buffer [{1}] in shader [{2}] that cannot be found in the "
					  "resource "
					  "cache.",
					  psl::utility::to_string(metaData.uid),
					  psl::utility::to_string(binding.buffer()),
					  psl::utility::to_string(stage.shader()));
					return;
				}
			} break;

			default:
				throw new std::runtime_error("This should not be reached");
				return;
			}
			++index;
		}
	}
	if(m_Textures.size() > 0) {
		glUseProgram(m_Program->id());
		for(const auto& [binding, texture] : m_Textures) {
			if(binding != std::numeric_limits<uint32_t>::max())
				glUniform1i(binding, binding);
		}
		glUseProgram(0);
	}
}

void material_t::bind() {
	if(!m_Program)
		return;
	glUseProgram(m_Program->id());
	const auto& blend_states = m_Data->blend_states();
	using namespace core::gfx::conversion;


	for(size_t i = 0; i < m_Textures.size(); ++i) {
		auto binding = m_Textures[i].first;

		if(binding == std::numeric_limits<uint32_t>::max())
			continue;

		glActiveTexture(GL_TEXTURE0 + binding);
		auto tex_id = m_Textures[i].second->id();
		glBindTexture(GL_TEXTURE_2D, tex_id);
		glBindSampler(binding, m_Samplers[i].second->id());
	}

	for(size_t i = 0; i < blend_states.size(); ++i) {
		glBlendEquationSeparatei(psl::narrow_cast<GLuint>(i),
								 to_gles(blend_states[i].color_blend_op()),
								 to_gles(blend_states[i].alpha_blend_op()));
		glBlendFuncSeparatei(psl::narrow_cast<GLuint>(i),
							 to_gles(blend_states[i].color_blend_src()),
							 to_gles(blend_states[i].color_blend_dst()),
							 to_gles(blend_states[i].alpha_blend_src()),
							 to_gles(blend_states[i].alpha_blend_dst()));
	}
	for(auto& buffer : m_Buffers) {
		if(buffer.offset == 0)
			glBindBufferBase(GL_UNIFORM_BUFFER, buffer.slot, buffer.buffer->id());
		else
			glBindBufferRange(GL_UNIFORM_BUFFER, buffer.slot, buffer.buffer->id(), buffer.offset, buffer.size);
	}

	switch(m_Data->cull_mode()) {
	case core::gfx::cullmode::front:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		break;
	case core::gfx::cullmode::back:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		break;
	case core::gfx::cullmode::all:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT_AND_BACK);
		break;
	case core::gfx::cullmode::none:
		glDisable(GL_CULL_FACE);
		break;
	}

	glDepthMask(m_Data->depth_write());
	if(m_Data->depth_test())
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	glGetError();
}

const std::vector<core::resource::handle<core::igles::shader>>& material_t::shaders() const noexcept {
	return m_Shaders;
}

const core::data::material_t& material_t::data() const noexcept {
	return m_Data.value();
}

bool material_t::bind_instance_data(uint32_t slot, uint32_t offset) {
	for(auto& buffer : m_Buffers) {
		if(buffer.slot == slot) {
			buffer.offset = offset;
			return true;
		}
	}
	core::igles::log->error("the requested binding slot {} was not found in the material", slot);
	return false;
}
