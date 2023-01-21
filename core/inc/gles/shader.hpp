#pragma once
#include "core/fwd/gles/shader.hpp"
#include "core/fwd/resource/resource.hpp"
#include "gles/types.hpp"
#include "psl/ustring.hpp"
#include <limits>

namespace core::igles {
class shader {
  public:
	shader(core::resource::cache_t& cache, const core::resource::metadata& metaData, core::meta::shader* metaFile);
	~shader();
	shader(const shader&)			 = delete;
	shader(shader&&)				 = delete;
	shader& operator=(const shader&) = delete;
	shader& operator=(shader&&)		 = delete;

	GLuint id() const noexcept;
	core::meta::shader* meta() const noexcept { return m_Meta; }

  private:
	GLuint m_Shader {std::numeric_limits<GLuint>::max()};
	core::meta::shader* m_Meta;
};
}	 // namespace core::igles
