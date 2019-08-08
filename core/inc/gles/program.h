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
		program(const psl::UID& uid, core::resource::cache& cache, core::resource::handle<core::data::material> data);
		~program();
		unsigned int id() const noexcept { return m_Program; }

	  private:
		unsigned int m_Program;
	};
} // namespace core::igles