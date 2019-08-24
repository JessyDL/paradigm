#include "data/sampler.h"
#include "resource/resource.hpp"
using namespace psl;
using namespace core::data;
using namespace core::resource;
using namespace core;

sampler::sampler(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile) noexcept{

}


bool sampler::mipmaps() const { return m_MipMapped.value; }
void sampler::mipmaps(bool value) { m_MipMapped.value = value; }

float sampler::mip_bias() const { return m_MipLodBias.value; }
void sampler::mip_bias(float value) { m_MipLodBias.value = value; }

gfx::sampler_mipmap_mode sampler::mip_mode() const { return m_MipMapMode.value; }
void sampler::mip_mode(gfx::sampler_mipmap_mode value) { m_MipMapMode.value = value; }

float sampler::mip_minlod() const { return m_MinLod.value; }
void sampler::mip_minlod(float value) { m_MinLod.value = value; }

float sampler::mip_maxlod() const { return m_MaxLod.value; }
void sampler::mip_maxlod(float value) { m_MaxLod.value = value; }

gfx::sampler_address_mode sampler::addressU() const { return m_AddressModeU.value; }
void sampler::addressU(gfx::sampler_address_mode value) { m_AddressModeU.value = value; }

gfx::sampler_address_mode sampler::addressV() const { return m_AddressModeV.value; }
void sampler::addressV(gfx::sampler_address_mode value) { m_AddressModeV.value = value; }

gfx::sampler_address_mode sampler::addressW() const { return m_AddressModeW.value; }
void sampler::addressW(gfx::sampler_address_mode value) { m_AddressModeW.value = value; }

gfx::border_color sampler::border_color() const { return m_BorderColor.value; }
void sampler::border_color(gfx::border_color value) { m_BorderColor.value = value; }

bool sampler::anisotropic_filtering() const { return m_AnisotropyEnable.value; }
void sampler::anisotropic_filtering(bool value) { m_AnisotropyEnable.value = value; }

float sampler::max_anisotropy() const { return m_MaxAnisotropy.value; }
void sampler::max_anisotropy(float value) { m_MaxAnisotropy.value = value; }

bool sampler::compare_mode() const { return m_CompareEnable.value; }
void sampler::compare_mode(bool value) { m_CompareEnable.value = value; }

gfx::compare_op sampler::compare_op() const { return m_CompareOp.value; }
void sampler::compare_op(gfx::compare_op value) { m_CompareOp.value = value; }

gfx::filter sampler::filter_min() const { return m_MinFilter.value; }
void sampler::filter_min(gfx::filter value) { m_MinFilter.value = value; }

gfx::filter sampler::filter_max() const { return m_MaxFilter.value; }
void sampler::filter_max(gfx::filter value) { m_MaxFilter.value = value; }

bool sampler::normalized_coordinates() const { return m_NormalizedCoordinates.value; }
void sampler::normalized_coordinates(bool value) { m_NormalizedCoordinates.value = value; }
