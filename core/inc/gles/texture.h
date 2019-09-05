#pragma once
#include "fwd/gles/texture.h"
#include "fwd/resource/resource.h"
#include <stdint.h>
#include "gles/types.h"

namespace gli
{
	class texture;
}
namespace core::igles
{
	class texture
	{
	  public:
		texture(core::resource::cache& cache, const core::resource::metadata& metaData, core::meta::texture* metaFile);
		~texture();

		texture(const texture& other)	 = delete;
		texture(texture&& other) noexcept = delete;
		texture& operator=(const texture& other) = delete;
		texture& operator=(texture&& other) noexcept = delete;

		GLuint id() const noexcept { return m_Texture; }

	  private:
		void load_2D();
		void create_2D(void* data = nullptr);
		void load_cube();


		gli::texture* m_TextureData{nullptr};
		core::resource::cache& m_Cache;
		core::meta::texture* m_Meta{nullptr};
		uint32_t m_MipLevels{0};
		GLuint m_Texture;
	};
} // namespace core::igles
