#pragma once
#include "vulkan_stdafx.h"
#include "systems/resource.h"
#include <unordered_map>
namespace core::meta
{
	class texture;
}

namespace gli
{
	class texture;
}

namespace core::ivk
{
	class context;
}

namespace core::gfx
{
	class buffer;

	/// \brief a texture resource used for rendering, either as target, or as input resource.
	///
	/// textures are vital in a graphics application, and this class abstracts 2D, 3D, etc..
	/// into one "texture object" as it is used by the renderer, and not to be used as a container
	/// directly.
	class texture
	{
	  public:
		texture(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
				core::resource::handle<core::ivk::context> context,
				core::resource::handle<core::gfx::buffer> stagingBuffer);
		texture(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
				core::resource::handle<core::ivk::context> context);
		~texture();

		/// \returns the vk::Image associated with this instance.
		const vk::Image& image() const noexcept;
		/// \returns the view into the vk::Image of this instance.
		const vk::ImageView& view() const noexcept;
		/// \returns the layout specifications.
		const vk::ImageLayout& layout() const noexcept;
		/// \returns the vulkan handle to the device memory location.
		const vk::DeviceMemory& memory() const noexcept;
		/// \returns the ImageSubresourceRange
		const vk::ImageSubresourceRange& subResourceRange() const noexcept;
		/// \returns the meta data that describes this texture object.
		const core::meta::texture& meta() const noexcept;
		/// \returns how many mip levels are present in this object.
		uint32_t mip_levels() const noexcept;
		/// \returns a descriptor image info for the given sampler, if none are present one is generated.
		/// \note these are maintained by the texture object itself and will live as long as the texture does.
		vk::DescriptorImageInfo& descriptor(const psl::UID& sampler);

	  private:
		void load_2D();
		void create_2D();
		void load_cube();

		gli::texture* m_TextureData;
		core::resource::handle<core::gfx::buffer> m_StagingBuffer;
		vk::Image m_Image;
		vk::ImageView m_View;
		vk::DeviceMemory m_DeviceMemory;
		vk::ImageLayout m_ImageLayout{vk::ImageLayout::eGeneral};
		vk::ImageSubresourceRange m_SubresourceRange;
		uint32_t m_MipLevels;

		core::resource::cache& m_Cache;
		core::resource::handle<core::ivk::context> m_Context;
		core::meta::texture* m_Meta;

		std::unordered_map<psl::UID, vk::DescriptorImageInfo*> m_Descriptors;
	};
} // namespace core::gfx
