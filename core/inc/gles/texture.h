#pragma once
#include "fwd/gles/texture.h"
#include "fwd/resource/resource.h"
#include "gles/types.h"
#include <stdint.h>

namespace gli
{
	class texture;
}
namespace core::igles
{
	class texture_t
	{
	  public:
		texture_t(core::resource::cache_t& cache, const core::resource::metadata& metaData, core::meta::texture_t* metaFile);
		~texture_t();

		texture_t(const texture_t& other)	  = delete;
		texture_t(texture_t&& other) noexcept = delete;
		texture_t& operator=(const texture_t& other) = delete;
		texture_t& operator=(texture_t&& other) noexcept = delete;

		GLuint id() const noexcept { return m_Texture; }

		const core::meta::texture_t& meta() const noexcept;

	  private:
		void load_2D();
		void create_2D(void* data = nullptr);
		void load_cube();


		gli::texture* m_TextureData {nullptr};
		core::resource::cache_t& m_Cache;
		core::meta::texture_t* m_Meta {nullptr};
		uint32_t m_MipLevels {0};
		GLuint m_Texture;
	};
}	 // namespace core::igles
