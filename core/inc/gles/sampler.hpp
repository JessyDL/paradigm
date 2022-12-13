#pragma once
#include "fwd/resource/resource.hpp"

namespace core::data
{
class sampler_t;
}	 // namespace core::data
namespace core::igles
{
class sampler_t
{
  public:
	sampler_t(core::resource::cache_t& cache,
			  const core::resource::metadata& metaData,
			  psl::meta::file* metaFile,
			  core::resource::handle<core::data::sampler_t> sampler_data);
	~sampler_t();

	sampler_t(const sampler_t& other)				 = delete;
	sampler_t(sampler_t&& other) noexcept			 = delete;
	sampler_t& operator=(const sampler_t& other)	 = delete;
	sampler_t& operator=(sampler_t&& other) noexcept = delete;

	unsigned int id() const noexcept { return m_Sampler; }

  private:
	unsigned int m_Sampler;
};
}	 // namespace core::igles
