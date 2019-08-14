#pragma once
#include "ustring.h"
#include "fwd/resource/resource.h"
#include "glad/glad_wgl.h"

namespace core::meta
{
	class shader;
}
namespace core::igles
{
	class shader
	{
	  public:
		shader(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile);
		~shader();
		shader(const shader&) = delete;
		shader(shader&&)	  = delete;
		shader& operator=(const shader&) = delete;
		shader& operator=(shader&&) = delete;

		GLuint id() const noexcept;
		core::meta::shader* meta() const noexcept { return m_Meta; }
	  private:
		GLuint m_Shader{std::numeric_limits<GLuint>::max()};
		core::meta::shader* m_Meta;
	};
} // namespace core::igles