#include "data/sampler.hpp"
#include "resource/resource.hpp"
#include <fmt/compile.h>
using namespace psl;
using namespace core::data;
using namespace core::resource;
using namespace core;

sampler_t::sampler_t(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile) noexcept
{}


bool sampler_t::mipmaps() const { return m_MipMapped.value; }
void sampler_t::mipmaps(bool value) { m_MipMapped.value = value; }

float sampler_t::mip_bias() const { return m_MipLodBias.value; }
void sampler_t::mip_bias(float value) { m_MipLodBias.value = value; }

gfx::sampler_mipmap_mode sampler_t::mip_mode() const { return m_MipMapMode.value; }
void sampler_t::mip_mode(gfx::sampler_mipmap_mode value) { m_MipMapMode.value = value; }

float sampler_t::mip_minlod() const { return m_MinLod.value; }
void sampler_t::mip_minlod(float value) { m_MinLod.value = value; }

float sampler_t::mip_maxlod() const { return m_MaxLod.value; }
void sampler_t::mip_maxlod(float value) { m_MaxLod.value = value; }

gfx::sampler_address_mode sampler_t::addressU() const { return m_AddressModeU.value; }
void sampler_t::addressU(gfx::sampler_address_mode value) { m_AddressModeU.value = value; }

gfx::sampler_address_mode sampler_t::addressV() const { return m_AddressModeV.value; }
void sampler_t::addressV(gfx::sampler_address_mode value) { m_AddressModeV.value = value; }

gfx::sampler_address_mode sampler_t::addressW() const { return m_AddressModeW.value; }
void sampler_t::addressW(gfx::sampler_address_mode value) { m_AddressModeW.value = value; }

void sampler_t::address(core::gfx::sampler_address_mode value) noexcept
{
	m_AddressModeU.value = value;
	m_AddressModeV.value = value;
	m_AddressModeW.value = value;
}

void sampler_t::address(core::gfx::sampler_address_mode u,
					  core::gfx::sampler_address_mode v,
					  core::gfx::sampler_address_mode w) noexcept
{
	m_AddressModeU.value = u;
	m_AddressModeV.value = v;
	m_AddressModeW.value = w;
}

gfx::border_color sampler_t::border_color() const { return m_BorderColor.value; }
void sampler_t::border_color(gfx::border_color value) { m_BorderColor.value = value; }

bool sampler_t::anisotropic_filtering() const { return m_AnisotropyEnable.value; }
void sampler_t::anisotropic_filtering(bool value) { m_AnisotropyEnable.value = value; }

float sampler_t::max_anisotropy() const { return m_MaxAnisotropy.value; }
void sampler_t::max_anisotropy(float value) { m_MaxAnisotropy.value = value; }

bool sampler_t::compare_mode() const { return m_CompareEnable.value; }
void sampler_t::compare_mode(bool value) { m_CompareEnable.value = value; }

gfx::compare_op sampler_t::compare_op() const { return m_CompareOp.value; }
void sampler_t::compare_op(gfx::compare_op value) { m_CompareOp.value = value; }

gfx::filter sampler_t::filter_min() const { return m_MinFilter.value; }
void sampler_t::filter_min(gfx::filter value) { m_MinFilter.value = value; }

gfx::filter sampler_t::filter_max() const { return m_MaxFilter.value; }
void sampler_t::filter_max(gfx::filter value) { m_MaxFilter.value = value; }

bool sampler_t::normalized_coordinates() const { return m_NormalizedCoordinates.value; }
void sampler_t::normalized_coordinates(bool value) { m_NormalizedCoordinates.value = value; }
