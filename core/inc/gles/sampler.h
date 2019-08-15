#pragma once
#include "fwd/resource/resource.h"

namespace core::data
{
	class sampler;
} // namespace core::data
namespace core::igles
{
	class sampler
	{
	  public:
		sampler(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				core::resource::handle<core::data::sampler> sampler_data);
		~sampler();

		sampler(const sampler& other)	 = delete;
		sampler(sampler&& other) noexcept = delete;
		sampler& operator=(const sampler& other) = delete;
		sampler& operator=(sampler&& other) noexcept = delete;

		unsigned int id() const noexcept { return m_Sampler; }
	  private:
		unsigned int m_Sampler;
	};
} // namespace core::igles