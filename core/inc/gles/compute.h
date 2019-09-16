#pragma once
#include "fwd/resource/resource.h"
#include "fwd/gles/compute.h"

namespace core::igles
{
	class compute
	{
	  public:
		compute(core::resource::cache& cache, const core::resource::metadata& metaData,
				psl::meta::file* metaFile);
		~compute();

		compute(const compute& other)	 = delete;
		compute(compute&& other) noexcept = delete;
		compute& operator=(const compute& other) = delete;
		compute& operator=(compute&& other) noexcept = delete;

		void dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z = 1u) const noexcept;
	  private:
		unsigned int m_Shader;
		unsigned int m_Program;
		core::meta::shader* m_Meta;
	};
}