#include "core/gles/compute.hpp"
#include "core/data/buffer.hpp"
#include "core/data/material.hpp"
#include "core/gles/buffer.hpp"
#include "core/gles/conversion.hpp"
#include "core/gles/igles.hpp"
#include "core/gles/program.hpp"
#include "core/gles/program_cache.hpp"
#include "core/gles/shader.hpp"
#include "core/gles/texture.hpp"
#include "core/meta/shader.hpp"
#include "core/resource/resource.hpp"

#include "core/gfx/buffer.hpp"

using namespace core::igles;
using namespace core::resource;
using namespace psl::meta;
using namespace core::gfx::conversion;

compute::compute(cache_t& cache,
				 const metadata& metaData,
				 file* metaFile,
				 core::resource::handle<core::data::material_t> data,
				 core::resource::handle<core::igles::program_cache> program_cache)
	: m_Meta(metaFile) {
	psl_assert(data->stages().size() == 1 && data->stages()[0].shader_stage() == core::gfx::shader_stage::compute,
			   "shader stages {} and stage name {} did not match assertion",
			   data->stages().size(),
			   psl::utility::to_string(data->stages()[0].shader_stage()));
	auto& stage		   = data->stages()[0];
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

	m_Program = program_cache->get(metaData.uid, data);

	auto meta = cache.library().get<core::meta::shader>(stage.shader()).value_or(nullptr);
	// now we validate the shader, and store all the bound resource handles
	auto index = 0;
	for(const auto& binding : stage.bindings()) {
		auto qualifier =
		  std::find_if(std::begin(shader_handle->meta()->descriptors()),
					   std::end(shader_handle->meta()->descriptors()),
					   [location = binding.binding_slot()](const core::meta::shader::descriptor& descriptor) {
						   return descriptor.binding() == location;
					   })
			->qualifier();
		switch(binding.descriptor()) {
		case core::gfx::binding_type::storage_image:
		case core::gfx::binding_type::combined_image_sampler: {
			auto binding_slot = glGetUniformLocation(m_Program->id(), meta->descriptors()[index].name().data());
			if(auto texture_handle = cache.find<core::igles::texture_t>(binding.texture()); texture_handle) {
				switch(qualifier) {
				case core::meta::shader::descriptor::dependency::in:
					m_InputTextures.push_back(std::make_pair(binding_slot, texture_handle));
					break;
				case core::meta::shader::descriptor::dependency::out:
					m_OutputTextures.push_back(std::make_pair(binding_slot, texture_handle));
					break;
				case core::meta::shader::descriptor::dependency::inout:
					m_InputTextures.push_back(std::make_pair(binding_slot, texture_handle));
					m_OutputTextures.push_back(std::make_pair(binding_slot, texture_handle));
					break;
				}
			} else {
				core::igles::log->error(
				  "igles::compute [{0}] uses a texture [{1}] in shader [{2}] that cannot be found in the "
				  "resource "
				  "cache.",
				  psl::utility::to_string(metaData.uid),
				  psl::utility::to_string(binding.texture()),
				  psl::utility::to_string(stage.shader()));
				return;
			}
		} break;
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

				auto usage = (binding.descriptor() == core::gfx::binding_type::uniform_buffer)
							   ? core::gfx::memory_usage::uniform_buffer
							   : core::gfx::memory_usage::storage_buffer;
				if(buffer_handle->buffer->data().usage() & usage) {
					switch(qualifier) {
					case core::meta::shader::descriptor::dependency::in:
						m_InputBuffers.push_back(std::make_pair(
						  binding.binding_slot(), buffer_handle->buffer->resource<gfx::graphics_backend::gles>()));
						break;
					case core::meta::shader::descriptor::dependency::out:
						m_OutputBuffers.push_back(std::make_pair(
						  binding.binding_slot(), buffer_handle->buffer->resource<gfx::graphics_backend::gles>()));
						break;
					case core::meta::shader::descriptor::dependency::inout:
						m_InputBuffers.push_back(std::make_pair(
						  binding.binding_slot(), buffer_handle->buffer->resource<gfx::graphics_backend::gles>()));
						m_OutputBuffers.push_back(std::make_pair(
						  binding.binding_slot(), buffer_handle->buffer->resource<gfx::graphics_backend::gles>()));
						break;
					}
				} else {
					core::igles::log->error(
					  "igles::compute [{0}] declares resource of the type [{1}], but we detected a resource of "
					  "the type [{2}] instead in shader [{3}]",
					  psl::utility::to_string(metaData.uid), /*vk::to_string(conversion::to_vk(binding.descriptor())),
					  vk::to_string(buffer_handle->data()->usage())*/
					  "",
					  "",
					  psl::utility::to_string(stage.shader()));
					return;
				}
			} else {
				core::igles::log->error(
				  "igles::compute [{0}] uses a buffer [{1}] in shader [{2}] that cannot be found in the "
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

compute::~compute() {}


void compute::dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z) const noexcept {
	glUseProgram(m_Program->id());

	for(const auto& [binding, texture] : m_InputTextures) {
		glActiveTexture(GL_TEXTURE0 + binding);
		glBindTexture(GL_TEXTURE_2D, texture->id());
	}

	for(const auto& [binding, buffer] : m_InputBuffers) {
		glBindBuffer(GL_UNIFORM_BUFFER, buffer->id());
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer->id());
	}

	for(const auto& [binding, texture] : m_OutputTextures) {
		glBindImageTexture(0, texture->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	}

	glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
	glGetError();
}
psl::array<core::resource::handle<core::igles::texture_t>> compute::textures() const noexcept {
	psl::array<core::resource::handle<core::igles::texture_t>> out;
	std::transform(std::begin(m_OutputTextures),
				   std::end(m_OutputTextures),
				   std::back_inserter(out),
				   [](const auto& pair) { return pair.second; });
	return out;
}
psl::array<core::resource::handle<core::igles::buffer_t>> compute::buffers() const noexcept {
	psl::array<core::resource::handle<core::igles::buffer_t>> out;
	std::transform(std::begin(m_OutputBuffers),
				   std::end(m_OutputBuffers),
				   std::back_inserter(out),
				   [](const auto& pair) { return pair.second; });
	return out;
}
