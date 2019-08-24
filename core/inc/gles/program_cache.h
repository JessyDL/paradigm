#pragma once
#include "fwd/resource/resource.h"

namespace core::data
{
	class material;
}

namespace core::igles
{
	class program;

	class program_cache
	{
	  public:
		program_cache(core::resource::cache& cache, const core::resource::metadata& metaData,
					  psl::meta::file* metaFile);
		~program_cache() = default;

		program_cache(const program_cache& other)	 = delete;
		program_cache(program_cache&& other) noexcept = delete;
		program_cache& operator=(const program_cache& other) = delete;
		program_cache& operator=(program_cache&& other) noexcept = delete;

		/// \brief allows you to get a pipeline that satisfy the material requirements
		/// \returns a handle to a pipeline object.
		/// \param[in] data the material containing the description of all bindings.
		core::resource::handle<core::igles::program> get(const psl::UID& uid,
														 core::resource::handle<core::data::material> data);

	  private:
		core::resource::cache* m_Cache;
	};
} // namespace core::igles