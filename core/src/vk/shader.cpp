#include "core/vk/shader.hpp"
#include "core/logging.hpp"
#include "core/meta/shader.hpp"
#include "core/vk/context.hpp"

using namespace psl;
using namespace core::gfx;
using namespace core::ivk;
using namespace core::resource;
using namespace core;

shader::shader(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   core::meta::shader* metaFile,
			   core::resource::handle<core::ivk::context> context)
	: m_Context(context), m_Cache(cache), m_UID(metaData.uid),
	  m_Meta(cache.library().get<core::meta::shader>(metaFile->ID()).value_or(nullptr)) {
	if(m_Meta == nullptr) {
		core::ivk::log->error("ivk::shader [{0}] does not have a valid, on disk, meta file", m_UID.to_string());
		return;
	}
	core::meta::shader& meta = *m_Meta;
	if(cache.library().is_physical_file(meta.ID())) {
		auto result = cache.library().load(meta.ID());
		if(!result) {
			core::ivk::log->error(
			  "could not load ivk::shader [{0}] from resource UID [{1}]", m_UID.to_string(), meta.ID().to_string());
		}

		vk::ShaderModuleCreateInfo moduleCreateInfo;
		moduleCreateInfo.pNext	  = NULL;
		moduleCreateInfo.codeSize = result.value().size();
		moduleCreateInfo.pCode	  = (uint32_t*)result.value().data();
		moduleCreateInfo.flags	  = vk::ShaderModuleCreateFlagBits();

		auto& [spec, pipe] = m_Specializations.emplace_back();
		pipe.stage		   = vk::ShaderStageFlagBits((uint32_t)meta.stage());

		core::ivk::log->info("creating ivk::shader [{0}] using resource uid [{1}] with shader stage [{2}]",
							 m_UID.to_string(),
							 meta.ID().to_string(),
							 vk::to_string(pipe.stage));

		spec.name  = "main";
		pipe.pName = spec.name.data();
		if(!core::utility::vulkan::check(m_Context->device().createShaderModule(&moduleCreateInfo, NULL, &pipe.module))) {
			m_Specializations.erase(std::end(m_Specializations) - 1u);
		}
	}
}
shader::shader(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   core::meta::shader* metaFile,
			   core::resource::handle<core::ivk::context> context,
			   const std::vector<specialization> specializations)
	: m_Context(context), m_Cache(cache), m_UID(metaData.uid) {
	if(cache.library().is_physical_file(m_UID)) {
		core::meta::shader& meta = *metaFile;
		auto result				 = cache.library().load(m_UID);
		if(!result) {
			core::ivk::log->error("ivk::shader [{0}] does not have a valid, on disk, meta file", m_UID.to_string());
		}

		vk::ShaderModuleCreateInfo moduleCreateInfo;
		moduleCreateInfo.pNext	  = NULL;
		moduleCreateInfo.codeSize = result.value().size();
		moduleCreateInfo.pCode	  = (uint32_t*)result.value().data();
		moduleCreateInfo.flags	  = vk::ShaderModuleCreateFlagBits();

		for(const auto& spec_temp : specializations) {
			auto& [spec, pipe] =
			  m_Specializations.emplace_back(std::make_pair(spec_temp, vk::PipelineShaderStageCreateInfo {}));

			pipe.stage = vk::ShaderStageFlagBits((uint32_t)meta.stage());

			core::ivk::log->info("creating ivk::shader [{0}] using resource uid [{1}] with shader stage [{2}]",
								 m_UID.to_string(),
								 meta.ID().to_string(),
								 vk::to_string(pipe.stage));

			if(!core::utility::vulkan::check(m_Context->device().createShaderModule(&moduleCreateInfo, NULL, &pipe.module))) {
				m_Specializations.erase(std::end(m_Specializations) - 1u);
			}
		}
	}

	for(auto& pair : m_Specializations) {
		pair.second.pName = pair.first.name.data();
	}
}

shader::~shader() {
	for(auto& [spec, pipeline] : m_Specializations) m_Context->device().destroyShaderModule(pipeline.module, nullptr);
}


std::optional<vk::PipelineShaderStageCreateInfo> shader::pipeline(const specialization& description) {
	auto it = std::find_if(std::begin(m_Specializations),
						   std::end(m_Specializations),
						   [&description](const std::pair<specialization, vk::PipelineShaderStageCreateInfo>& pair) {
							   return pair.first == description;
						   });

	if(it != std::end(m_Specializations))
		return it->second;

	if(!m_Cache.library().is_physical_file(m_UID))
		return {};

	auto result = m_Cache.library().load(m_UID);

	vk::ShaderModuleCreateInfo moduleCreateInfo;
	moduleCreateInfo.pNext	  = NULL;
	moduleCreateInfo.codeSize = result.value().size();
	moduleCreateInfo.pCode	  = (uint32_t*)result.value().data();
	moduleCreateInfo.flags	  = vk::ShaderModuleCreateFlagBits();

	auto& [spec, pipe] =
	  m_Specializations.emplace_back(std::make_pair(description, vk::PipelineShaderStageCreateInfo {}));
	pipe.stage = m_Specializations[0].second.stage;
	pipe.pName = description.name.data();


	for(auto& pair : m_Specializations) {
		pair.second.pName = pair.first.name.data();
	}

	if(!core::utility::vulkan::check(m_Context->device().createShaderModule(&moduleCreateInfo, NULL, &pipe.module))) {
		m_Specializations.erase(std::end(m_Specializations) - 1u);
		return {};
	}

	return pipe;
}
core::meta::shader* shader::meta() const noexcept {
	return m_Meta;
}
