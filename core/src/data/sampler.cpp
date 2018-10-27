#include "stdafx.h"
#include "data/sampler.h"

using namespace core::data;
using namespace core::resource;

sampler::sampler(const UID& uid, cache& cache)
{

}


bool sampler::mipmaps() const { return m_MipMapped.value; }
void sampler::mipmaps(bool value) { m_MipMapped.value = value; }

float sampler::mip_bias() const { return m_MipLodBias.value; }
void sampler::mip_bias(float value) { m_MipLodBias.value = value; }

vk::SamplerMipmapMode sampler::mip_mode() const { return m_MipMapMode.value; }
void sampler::mip_mode(vk::SamplerMipmapMode value) { m_MipMapMode.value = value; }

float sampler::mip_minlod() const { return m_MinLod.value; }
void sampler::mip_minlod(float value) { m_MinLod.value = value; }

float sampler::mip_maxlod() const { return m_MaxLod.value; }
void sampler::mip_maxlod(float value) { m_MaxLod.value = value; }

vk::SamplerAddressMode sampler::addressU() const { return m_AddressModeU.value; }
void sampler::addressU(vk::SamplerAddressMode value) { m_AddressModeU.value = value; }

vk::SamplerAddressMode sampler::addressV() const { return m_AddressModeV.value; }
void sampler::addressV(vk::SamplerAddressMode value) { m_AddressModeV.value = value; }

vk::SamplerAddressMode sampler::addressW() const { return m_AddressModeW.value; }
void sampler::addressW(vk::SamplerAddressMode value) { m_AddressModeW.value = value; }

vk::BorderColor sampler::border_color() const { return m_BorderColor.value; }
void sampler::border_color(vk::BorderColor value) { m_BorderColor.value = value; }

bool sampler::anisotropic_filtering() const { return m_AnisotropyEnable.value; }
void sampler::anisotropic_filtering(bool value) { m_AnisotropyEnable.value = value; }

float sampler::max_anisotropy() const { return m_MaxAnisotropy.value; }
void sampler::max_anisotropy(float value) { m_MaxAnisotropy.value = value; }

bool sampler::compare_mode() const { return m_CompareEnable.value; }
void sampler::compare_mode(bool value) { m_CompareEnable.value = value; }

vk::CompareOp sampler::compare_op() const { return m_CompareOp.value; }
void sampler::compare_op(vk::CompareOp value) { m_CompareOp.value = value; }

vk::Filter sampler::filter_min() const { return m_MinFilter.value; }
void sampler::filter_min(vk::Filter value) { m_MinFilter.value = value; }

vk::Filter sampler::filter_max() const { return m_MaxFilter.value; }
void sampler::filter_max(vk::Filter value) { m_MaxFilter.value = value; }

bool sampler::normalized_coordinates() const { return m_NormalizedCoordinates.value; }
void sampler::normalized_coordinates(bool value) { m_NormalizedCoordinates.value = value; }
