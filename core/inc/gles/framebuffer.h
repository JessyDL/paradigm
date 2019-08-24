#pragma once
#include "resource/handle.h"
#include "gfx/types.h"
#include "array.h"

namespace core::data
{
	class framebuffer;
}
namespace core::igles
{
	class texture;
	class sampler;

	class framebuffer
	{

	  public:
		struct attachment
		{
			unsigned int texture;
		};
		struct description
		{
			core::gfx::format format;
		};
		struct binding
		{
			psl::array<attachment> attachments;
			description description;
			uint32_t index;
		};

		framebuffer(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
					core::resource::handle<core::data::framebuffer> data);
		~framebuffer();

		framebuffer(const framebuffer& other)	 = delete;
		framebuffer(framebuffer&& other) noexcept = delete;
		framebuffer& operator=(const framebuffer& other) = delete;
		framebuffer& operator=(framebuffer&& other) noexcept = delete;

	  private:
		std::vector<core::resource::handle<core::igles::texture>> m_Textures;
		std::vector<binding> m_Bindings;
		core::resource::handle<core::igles::sampler> m_Sampler;
		core::resource::handle<core::data::framebuffer> m_Data;

		std::vector<unsigned int> m_Framebuffers;
	};
} // namespace core::igles