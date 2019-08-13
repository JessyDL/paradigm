#pragma once
#include "resource/resource.hpp"
#include "gfx/types.h"
#include "resource/c2.h"
#include "meta/texture.h"

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
		using meta_type = core::meta::texture;

		texture(core::r2::cache& cache, const core::r2::metadata& metaData, meta_type* metaFile) : m_Cache(*(core::resource::cache*)(nullptr))
		{
			m_Meta = metaFile;
		};
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