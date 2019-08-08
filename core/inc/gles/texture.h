#pragma once
#include "resource/resource.hpp"
#include "gfx/types.h"

namespace core::meta
{
	class texture;
}
namespace gli
{
	class texture;
}
namespace core::igles
{
	class texture
	{
	  public:
		texture(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile);
		~texture();

		texture(const texture& other)	 = delete;
		texture(texture&& other) noexcept = delete;
		texture& operator=(const texture& other) = delete;
		texture& operator=(texture&& other) noexcept = delete;

		GLuint id() const noexcept { return m_Texture; }
	  private:
		void load_2D();
		void create_2D();
		void load_cube();

		
		gli::texture* m_TextureData{nullptr};
		core::resource::cache& m_Cache;
		core::meta::texture* m_Meta{nullptr};
		uint32_t m_MipLevels{0};
		GLuint m_Texture;
	};
} // namespace core::igles