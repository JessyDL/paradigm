#pragma once
#include "fwd/resource/resource.h"

namespace core::data
{
	class material;
}

namespace core::igles
{
	class program
	{
	  public:
		program(core::resource::cache& cache,
				const core::resource::metadata& metaData,
				psl::meta::file* metaFile,
				core::resource::handle<core::data::material> data);
		~program();
		unsigned int id() const noexcept { return m_Program; }

	  private:
		unsigned int m_Program;
	};
}	 // namespace core::igles