#pragma once
#include "resource/resource.hpp"
#include "vk/ivk.hpp"
#include <vector>

namespace core::data
{
	class framebuffer_t;
}
namespace core::ivk
{
	class texture_t;
	class context;
	class sampler_t;
}	 // namespace core::ivk

namespace core::ivk
{
	/// \brief describes a set of images to use as rendertargets
	///
	/// in many graphics applications you will need to use more advanced techniques
	/// than just rendering into the backbuffer (swapchain), and to do that you will need
	/// to describe a set of images to the driver that you will use as render targets.
	/// the framebuffer class is just that, and allows you to bundle together images to do
	/// postprocessing, or shadowmapping, etc...
	class framebuffer_t final
	{
	  public:
		using texture_handle = core::resource::handle<core::ivk::texture_t>;

		/// \brief describes a single binding point (can be many) in a framebuffer.
		///
		/// although most framebuffers are fairly simplistic, you might end up in scenarios
		/// where you will need to have multiple layers in your framebuffer.
		/// this is where bindings come in; even though most bindings will have 1 attachment,
		/// in a multi-layer scenario you can have many attachments, or have some indices all share
		/// the same attachment.
		struct binding
		{
			std::vector<texture_handle> attachments;
			vk::AttachmentDescription description;
			uint32_t index;
		};

		framebuffer_t(core::resource::cache_t& cache,
					const core::resource::metadata& metaData,
					psl::meta::file* metaFile,
					core::resource::handle<core::ivk::context> context,
					core::resource::handle<core::data::framebuffer_t> data);
		framebuffer_t(const framebuffer_t&) = delete;
		framebuffer_t(framebuffer_t&&)		= delete;
		framebuffer_t& operator=(const framebuffer_t&) = delete;
		framebuffer_t& operator=(framebuffer_t&&) = delete;
		~framebuffer_t();

		/// \returns all attachments for a specific index
		/// \param[in] index the index to return the attachments for.
		std::vector<texture_handle> attachments(uint32_t index = 0u) const noexcept;

		/// \returns all color attachments for a specific index
		/// \param[in] index the index to return the attachments for.
		std::vector<texture_handle> color_attachments(uint32_t index = 0u) const noexcept;

		/// \returns the sampler resource associated with this framebuffer.
		core::resource::handle<core::ivk::sampler_t> sampler() const noexcept;
		/// \returns the data used to create this framebuffer
		core::resource::handle<core::data::framebuffer_t> data() const noexcept;
		/// \returns the renderpass this framebuffer created and manages.
		vk::RenderPass render_pass() const noexcept;
		/// \returns all vulkan framebuffer objects that constitute this framebuffer.
		const std::vector<vk::Framebuffer>& framebuffers() const noexcept;
		/// \returns the image descriptor.
		vk::DescriptorImageInfo descriptor() const noexcept;

	  private:
		bool add(core::resource::cache_t& cache,
				 const psl::UID& uid,
				 vk::AttachmentDescription description,
				 size_t index,
				 size_t count);

		std::vector<binding> m_Bindings;
		core::resource::handle<core::ivk::sampler_t> m_Sampler;
		core::resource::handle<core::data::framebuffer_t> m_Data;
		core::resource::handle<core::ivk::context> m_Context;
		vk::RenderPass m_RenderPass;
		std::vector<vk::Framebuffer> m_Framebuffers;
		vk::DescriptorImageInfo m_Descriptor;
	};
}	 // namespace core::ivk
