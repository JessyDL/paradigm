
#include "core/vk/sampler.hpp"
#include "core/data/sampler.hpp"
#include "core/vk/context.hpp"
#include "core/vk/conversion.hpp"

using namespace psl;
using namespace core::gfx::conversion;
using namespace core::ivk;
using namespace core::resource;
using namespace core;

sampler_t::sampler_t(core::resource::cache_t& cache,
					 const core::resource::metadata& metaData,
					 psl::meta::file* metaFile,
					 handle<context> context,
					 handle<data::sampler_t> sampler_data)
	: m_Data(sampler_data), m_Context(context), m_Samplers {} {
	size_t iterationCount = (m_Data->mipmaps()) ? 14 : 1;	 // 14 == 8096 // todo: this is a hack
	m_Samplers.resize(iterationCount);
	for(size_t i = 0u; i < iterationCount; ++i) {
		vk::SamplerCreateInfo sampler;
		sampler.magFilter	  = to_vk(m_Data->filter_max());
		sampler.minFilter	  = to_vk(m_Data->filter_min());
		sampler.mipmapMode	  = to_vk(m_Data->mip_mode());
		sampler.addressModeU  = to_vk(m_Data->addressU());
		sampler.addressModeV  = to_vk(m_Data->addressV());
		sampler.addressModeW  = to_vk(m_Data->addressW());
		sampler.mipLodBias	  = m_Data->mip_bias();
		sampler.compareEnable = m_Data->compare_mode();
		sampler.compareOp	  = to_vk(m_Data->compare_op());
		sampler.minLod		  = m_Data->mip_minlod();
		sampler.maxLod		  = (m_Data->mipmaps()) ? i : 1.0f;	   // todo: figure this out more correctly;
		// Enable anisotropic filtering
		sampler.maxAnisotropy	 = m_Data->max_anisotropy();
		sampler.anisotropyEnable = m_Data->anisotropic_filtering();
		sampler.borderColor		 = to_vk(m_Data->border_color());
		utility::vulkan::check(m_Context->device().createSampler(&sampler, nullptr, &m_Samplers[i]));
	}
}

sampler_t::~sampler_t() {
	for(auto& sampler : m_Samplers) m_Context->device().destroySampler(sampler, nullptr);
}

const vk::Sampler& sampler_t::get(size_t mip) const noexcept {
	if(mip >= m_Samplers.size()) {
		LOG_WARNING("Requested a mip level higher than the available mip levels");
		return m_Samplers[m_Samplers.size() - 1];
	}
	return m_Samplers[mip];
}

const core::data::sampler_t& sampler_t::data() const noexcept {
	return m_Data.value();
}
