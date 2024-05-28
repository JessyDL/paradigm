#include "core/wgpu/pipeline.hpp"
#include "core/data/material.hpp"
#include "core/meta/shader.hpp"
#include "core/resource/cache.hpp"
#include "core/wgpu/context.hpp"
#include "core/wgpu/conversion.hpp"
#include <psl/meta.hpp>

namespace core::iwgpu {
pipeline::pipeline(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   core::resource::handle<core::iwgpu::context> context,
				   core::resource::handle<core::data::material_t> data)
	: m_Cache(cache), m_Meta(metaFile), m_UID(metaData.uid), m_Context(context) {
	auto pipelineDesc  = wgpu::PipelineLayoutDescriptor {};
	const auto id_str  = metaFile->ID().to_string();
	pipelineDesc.label = id_str.c_str();

	std::vector<wgpu::BindGroupLayout> bindGroupLayouts {};
	std::vector<wgpu::BindGroupLayoutEntry> bindGroupLayoutEntries {};

	std::vector<core::meta::shader*> shaders {};
	std::for_each(std::begin(data->stages()), std::end(data->stages()), [&](const auto& stage) {
		core::meta::shader* shader_handle = cache.library().get<core::meta::shader>(stage.shader()).value_or(nullptr);
		if(!shader_handle) {
			core::iwgpu::log->error("tried to load incorrect shader {}", psl::utility::to_string(stage.shader()));
			return;
		}
		shaders.push_back(shader_handle);
	});

	for(auto const& stage : data->stages()) {
		auto shader_handle = cache.library().get<core::meta::shader>(stage.shader()).value_or(nullptr);
		if(!shader_handle) {
			core::iwgpu::log->error("tried to load incorrect shader {}", psl::utility::to_string(stage.shader()));
			return;
		}

		for(auto& attribute : shader_handle->inputs()) {
			auto& binding	   = bindGroupLayoutEntries.emplace_back();
			binding.binding	   = attribute.location();
			binding.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
		}
	}
	auto rpipelineDesc	 = wgpu::RenderPipelineDescriptor {};
	rpipelineDesc.layout = m_Context->device().CreatePipelineLayout(&pipelineDesc);

	psl::not_implemented(130);

	/*auto pipelineDesc  = wgpu::RenderPipelineDescriptor {};
	const auto id_str  = metaFile->ID().to_string();
	pipelineDesc.label = id_str.c_str();

	std::vector<wgpu::VertexBufferLayout> vertexBuffers {};
	std::vector<wgpu::VertexAttribute> vertexAttributes {};

	std::vector<core::meta::shader*> shaders {};
	std::for_each(std::begin(data->stages()), std::end(data->stages()), [&](const auto& stage) {
		core::meta::shader* shader_handle = cache.library().get<core::meta::shader>(stage.shader()).value_or(nullptr);
		if(!shader_handle) {
			core::iwgpu::log->error("tried to load incorrect shader {}", psl::utility::to_string(stage.shader()));
			return;
		}
		shaders.push_back(shader_handle);
	});

	for(auto const& stage : data->stages()) {
		auto shader_handle = cache.library().get<core::meta::shader>(stage.shader()).value_or(nullptr);
		if(!shader_handle) {
			core::iwgpu::log->error("tried to load incorrect shader {}", psl::utility::to_string(stage.shader()));
			return;
		}

		if(stage.shader_stage() != core::gfx::shader_stage::vertex &&
		   stage.shader_stage() != core::gfx::shader_stage::compute)
			continue;

		for(auto& attribute : shader_handle->inputs()) {
			auto& binding = vertexBuffers.emplace_back();

			binding.arrayStride = attribute.size();


			auto input_rate = std::find_if(std::begin(stage.attributes()),
										   std::end(stage.attributes()),
										   [location = attribute.location()](const auto& attribute) noexcept {
											   return location == attribute.location();
										   });
			binding.stepMode =
			  core::gfx::conversion::to_wgpu(input_rate->input_rate().value_or(core::gfx::vertex_input_rate::vertex));

			const auto offset = vertexAttributes.size();

			for(auto i = 0; i < attribute.count(); ++i) {
				auto& attribute_desc		  = vertexAttributes.emplace_back();
				attribute_desc.shaderLocation = attribute.location() + i;
				attribute_desc.format		  = core::gfx::conversion::to_wgpu(attribute.format());
				attribute_desc.offset		  = attribute.size() * i;
			}

			binding.attributes	   = vertexAttributes.data() + offset;
			binding.attributeCount = attribute.count();
		}
	}

	pipelineDesc.vertex.bufferCount = vertexBuffers.size();
	pipelineDesc.vertex.buffers		= vertexBuffers.data();
	pipelineDesc.vertex.module 	= shaders[0]->handle();

	auto pipeline = m_Context->device().CreateRenderPipeline(&pipelineDesc);*/
}
}	 // namespace core::iwgpu
