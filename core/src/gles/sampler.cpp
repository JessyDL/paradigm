#include "gles/sampler.h"
#include "data/sampler.h"
#include "resource/resource.hpp"
#include "gfx/types.h"

using namespace core::resource;
using namespace core::igles;
namespace data = core::data;

sampler::sampler(const psl::UID& uid, cache& cache, handle<data::sampler> sampler_data)
{
	size_t iterationCount = (sampler_data->mipmaps()) ? 14 : 1; // 14 == 8096 // todo: this is a hack

	glGenSamplers(1, &m_Sampler);

	// todo: MIN and MAG filter have no mipmapmode equivalent in GLES
	glSamplerParameteri(m_Sampler, GL_TEXTURE_MIN_FILTER, to_gles(sampler_data->filter_min()));
	glSamplerParameteri(m_Sampler, GL_TEXTURE_MAG_FILTER, to_gles(sampler_data->filter_max()));
	glSamplerParameteri(m_Sampler, GL_TEXTURE_WRAP_S, to_gles(sampler_data->addressU()));
	glSamplerParameteri(m_Sampler, GL_TEXTURE_WRAP_T, to_gles(sampler_data->addressV()));
	glSamplerParameteri(m_Sampler, GL_TEXTURE_WRAP_R, to_gles(sampler_data->addressW()));

	if(sampler_data->anisotropic_filtering())
		glSamplerParameterf(m_Sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler_data->max_anisotropy());

	to_gles(sampler_data->border_color(), m_Sampler);

	glSamplerParameterf(m_Sampler, GL_TEXTURE_MIN_LOD, sampler_data->mip_minlod());
	glSamplerParameterf(m_Sampler, GL_TEXTURE_MAX_LOD, sampler_data->mip_maxlod());

	glSamplerParameteri(m_Sampler, GL_TEXTURE_COMPARE_FUNC, to_gles(sampler_data->compare_op()));
	// todo find GL_TEXTURE_LOD_BIAS equivalent GLES
	// todo figure out use case for GL_TEXTURE_COMPARE_MODE
}

sampler::~sampler() { glDeleteSamplers(1, &m_Sampler); }