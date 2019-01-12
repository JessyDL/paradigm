#pragma once
#include "vulkan_stdafx.h"
#include <vector>
#include "systems/resource.h"

namespace core::data
{
	class framebuffer;
}
namespace core::gfx
{
	class texture;
	class sampler;
	class context;

	/// \brief describes a set of images to use as rendertargets
	///
	/// in many graphics applications you will need to use more advanced techniques
	/// than just rendering into the backbuffer (swapchain), and to do that you will need
	/// to describe a set of images to the driver that you will use as render targets.
	/// the framebuffer class is just that, and allows you to bundle together images to do
	/// postprocessing, or shadowmapping, etc... 
	class framebuffer final
	{
	public:
		/// \brief describes a single attachment in a framebuffer.
		struct attachment
		{
			vk::Image image;
			vk::DeviceMemory memory;
			vk::ImageView view;
			vk::ImageSubresourceRange subresourceRange;
		};

		/// \brief describes a single binding point (can be many) in a framebuffer.
		///
		/// although most framebuffers are fairly simplistic, you might end up in scenarios
		/// where you will need to have multiple layers in your framebuffer.
		/// this is where bindings come in; even though most bindings will have 1 attachment,
		/// in a multi-layer scenario you can have many attachments, or have some indices all share
		/// the same attachment.
		struct binding
		{
			std::vector<attachment> attachments;
			vk::AttachmentDescription description;
			uint32_t index;
		};

		framebuffer(const psl::UID& uid, core::resource::cache& cache, core::resource::handle<core::gfx::context> context, core::resource::handle<core::data::framebuffer> data);
		framebuffer(const framebuffer&) = delete;
		framebuffer(framebuffer&&) = delete;
		framebuffer& operator=(const framebuffer&) = delete;
		framebuffer& operator=(framebuffer&&) = delete;
		~framebuffer();

		/// \returns all attachments for a specific index
		/// \param[in] index the index to return the attachments for.
		std::vector<attachment> attachments(uint32_t index = 0u) const noexcept;

		/// \returns the sampler resource associated with this framebuffer.
		core::resource::handle<core::gfx::sampler> sampler() const noexcept;
		/// \returns the data used to create this framebuffer
		core::resource::handle<core::data::framebuffer> data() const noexcept;
		/// \returns the renderpass this framebuffer created and manages.
		vk::RenderPass render_pass() const noexcept;
		/// \returns all vulkan framebuffer objects that constitute this framebuffer.
		const std::vector<vk::Framebuffer>& framebuffers() const noexcept;
		/// \returns the image descriptor.
		vk::DescriptorImageInfo descriptor() const noexcept;
	private:
		bool add(core::resource::cache& cache, const psl::UID& uid, vk::AttachmentDescription description, size_t index, size_t count);

		std::vector<core::resource::handle<core::gfx::texture>> m_Textures;
		std::vector<binding> m_Bindings;
		core::resource::handle<core::gfx::sampler> m_Sampler;
		core::resource::handle<core::data::framebuffer> m_Data;
		core::resource::handle<core::gfx::context> m_Context;
		vk::RenderPass m_RenderPass;
		std::vector<vk::Framebuffer> m_Framebuffers;
		vk::DescriptorImageInfo m_Descriptor;
	};
}
