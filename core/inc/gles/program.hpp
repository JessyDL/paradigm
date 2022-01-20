#pragma once
#include "fwd/resource/resource.hpp"

namespace core::data
{
	class material_t;
}

namespace core::igles
{
	class program
	{
	  public:
		program(core::resource::cache_t& cache,
				const core::resource::metadata& metaData,
				psl::meta::file* metaFile,
				core::resource::handle<core::data::material_t> data);
		~program();
		unsigned int id() const noexcept { return m_Program; }

	  private:
		unsigned int m_Program;
	};
}	 // namespace core::igles