#include "core/wgpu/shader.hpp"
#include "core/meta/shader.hpp"
#include "core/wgpu/context.hpp"
#include "core/wgpu/conversion.hpp"

#include "core/resource/cache.hpp"
#include "core/resource/handle.hpp"

namespace core::iwgpu {

shader::shader(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   core::meta::shader* metaFile,
			   core::resource::handle<core::iwgpu::context> context)
	: m_Context(context), m_Cache(cache), m_Meta(metaFile), m_UID(metaData.uid) {
	auto meta	= cache.library().get<core::meta::shader>(metaFile->ID()).value_or(nullptr);
	m_Meta		= meta;
	auto result = cache.library().load(meta->ID());
	if(!result) {
		core::iwgpu::log->error("could not load igles::shader [{0}] from resource UID [{1}]",
								metaData.uid.to_string(),
								meta->ID().to_string());
		return;
	}

	auto const shader_stage = gfx::conversion::to_wgpu(meta->stage());

	wgpu::ShaderModuleDescriptor descriptor {};
	auto const id_str = meta->ID().to_string();
	descriptor.label  = id_str.data();

	switch(meta->source_format()) {
	case core::gfx::shader_source_format::spirv: {
		wgpu::ShaderModuleSPIRVDescriptor spirvDescriptor {};
		spirvDescriptor.nextInChain = nullptr;
		spirvDescriptor.code		= reinterpret_cast<uint32_t const*>(result.value().data());
		spirvDescriptor.codeSize	= result.value().size() / sizeof(uint32_t);
		descriptor.nextInChain		= &spirvDescriptor;


		m_Shader = m_Context->device().CreateShaderModule(&descriptor);
	} break;
	case core::gfx::shader_source_format::unknown:
	case core::gfx::shader_source_format::wgsl: {
		wgpu::ShaderModuleWGSLDescriptor wgslDescriptor {};
		wgslDescriptor.nextInChain = nullptr;
		wgslDescriptor.code		   = result.value().data();
		descriptor.nextInChain	   = &wgslDescriptor;

		m_Shader = m_Context->device().CreateShaderModule(&descriptor);
	} break;
	default:
		core::iwgpu::log->error("unsupported shader source format for shader [{0}] from resource UID [{1}]",
								metaData.uid.to_string(),
								meta->ID().to_string());
		return;
	}

	if(!m_Shader) {
		core::iwgpu::log->error("could not create shader module for shader [{0}] from resource UID [{1}]",
								metaData.uid.to_string(),
								meta->ID().to_string());
		return;
	}
}
}	 // namespace core::iwgpu
