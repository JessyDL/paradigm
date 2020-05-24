#pragma once
#include "resource/handle.h"
#include "gfx/types.h"
#include "psl/array.h"

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
		using texture_handle = core::resource::handle<core::igles::texture>;
		struct description
		{
			core::gfx::format format;
		};
		struct binding
		{
			psl::array<texture_handle> attachments;
			description description;
			uint32_t index;
		};

		framebuffer(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
					core::resource::handle<core::data::framebuffer> data);
		~framebuffer();

		framebuffer(const framebuffer& other)	  = delete;
		framebuffer(framebuffer&& other) noexcept = delete;
		framebuffer& operator=(const framebuffer& other) = delete;
		framebuffer& operator=(framebuffer&& other) noexcept = delete;

		/// \returns all attachments for a specific index
		/// \param[in] index the index to return the attachments for.
		std::vector<texture_handle> attachments(uint32_t index = 0u) const noexcept;

		/// \returns all color attachments for a specific index
		/// \param[in] index the index to return the attachments for.
		std::vector<texture_handle> color_attachments(uint32_t index = 0u) const noexcept;

		/// \returns the sampler resource associated with this framebuffer.
		core::resource::handle<core::igles::sampler> sampler() const noexcept;
		/// \returns the data used to create this framebuffer
		core::resource::handle<core::data::framebuffer> data() const noexcept;

		const std::vector<unsigned int>& framebuffers() const noexcept;

	  private:
		std::vector<binding> m_Bindings;
		core::resource::handle<core::igles::sampler> m_Sampler;
		core::resource::handle<core::data::framebuffer> m_Data;

		std::vector<unsigned int> m_Framebuffers;
	};
} // namespace core::igles