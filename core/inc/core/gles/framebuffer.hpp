#pragma once
#include "core/gfx/types.hpp"
#include "core/resource/handle.hpp"
#include "psl/array.hpp"

namespace core::data {
class framebuffer_t;
}
namespace core::igles {
class texture_t;
class sampler_t;

class framebuffer_t {
  public:
	using texture_handle = core::resource::handle<core::igles::texture_t>;
	struct description {
		core::gfx::format_t format;
	};
	struct binding {
		psl::array<texture_handle> attachments;
		description description;
		uint32_t index;
	};

	framebuffer_t(core::resource::cache_t& cache,
				  const core::resource::metadata& metaData,
				  psl::meta::file* metaFile,
				  core::resource::handle<core::data::framebuffer_t> data);
	~framebuffer_t();

	framebuffer_t(const framebuffer_t& other)				 = delete;
	framebuffer_t(framebuffer_t&& other) noexcept			 = delete;
	framebuffer_t& operator=(const framebuffer_t& other)	 = delete;
	framebuffer_t& operator=(framebuffer_t&& other) noexcept = delete;

	/// \returns all attachments for a specific index
	/// \param[in] index the index to return the attachments for.
	std::vector<texture_handle> attachments(uint32_t index = 0u) const noexcept;

	/// \returns all color attachments for a specific index
	/// \param[in] index the index to return the attachments for.
	std::vector<texture_handle> color_attachments(uint32_t index = 0u) const noexcept;

	/// \returns the sampler resource associated with this framebuffer.
	core::resource::handle<core::igles::sampler_t> sampler() const noexcept;
	/// \returns the data used to create this framebuffer
	core::resource::handle<core::data::framebuffer_t> data() const noexcept;

	const std::vector<unsigned int>& framebuffers() const noexcept;

  private:
	std::vector<binding> m_Bindings;
	core::resource::handle<core::igles::sampler_t> m_Sampler;
	core::resource::handle<core::data::framebuffer_t> m_Data;

	std::vector<unsigned int> m_Framebuffers;
};
}	 // namespace core::igles
