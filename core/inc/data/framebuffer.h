#pragma once
#include "serialization.h"
#include "vulkan_stdafx.h"
#include "meta.h"
#include "systems/resource.h"


namespace core::gfx
{
	class sampler;
}
namespace core::resource
{
	class cache;
}
namespace core::data
{
	/// \brief container class that describes the data to create a set of rendertargets.
	class framebuffer final
	{
		friend class psl::serialization::accessor;
	public:
		/// \brief describes a single rendertarget in a framebuffer.
		///
		/// all data contained within this object is not guaranteed to be loaded, this includes the psl::UID of the core::gfx::texture.
		/// You should take caution when calling methods that manipulate the contained resources for this reason.
		class attachment final
		{
			friend class psl::serialization::accessor;
			friend class framebuffer;
		public:
			/// \brief describes how the rendertarget should be loaded, stored, it's sample count, and what transition layouts it should go through.
			class description final
			{
				friend class psl::serialization::accessor;
			public:
				/// \brief defaulted constructor
				description() = default;
				/// \brief construction based on a vk::AttachmentDescription.
				description(vk::AttachmentDescription descr);
				/// \brief returns a vk::AttachmentDescription based on the internal settings.
				/// \returns a vk::AttachmentDescription based on the internal settings.
				vk::AttachmentDescription vkDescription() const;
			private:
				template <typename S>
				void serialize(S& serializer)
				{
					serializer << m_SampleCountFlags << m_LoadOp << m_StoreOp << m_StencilLoadOp << m_StencilStoreOp << m_InitialLayout << m_FinalLayout << m_Format;
				}
				static constexpr const char serialization_name[12]{"DESCRIPTION"};
				psl::serialization::property<vk::SampleCountFlagBits, const_str("SAMPLE_COUNT_FLAGS", 18)> m_SampleCountFlags;
				psl::serialization::property<vk::AttachmentLoadOp, const_str("LOAD_OP", 7)> m_LoadOp;
				psl::serialization::property<vk::AttachmentStoreOp, const_str("STORE_OP", 8)> m_StoreOp;
				psl::serialization::property<vk::AttachmentLoadOp, const_str("STENCIl_LOAD_OP", 15)> m_StencilLoadOp;
				psl::serialization::property<vk::AttachmentStoreOp, const_str("STENCIl_STORE_OP", 16)> m_StencilStoreOp;
				psl::serialization::property<vk::ImageLayout, const_str("INITIAL_LAYOUT", 14)> m_InitialLayout;
				psl::serialization::property<vk::ImageLayout, const_str("FINAL_LAYOUT", 12)> m_FinalLayout;
				psl::serialization::property<vk::Format, const_str("FORMAT", 6)> m_Format;

			};
			/// \brief constructs an attachment based on the given texture and values.
			///
			/// \note that the texture should be valid (not pointing to a non-texture),
			/// otherwise you might run into issues further down.
			/// \param[in] texture a psl::UID pointing to a valid (known or constructed), core::gfx::texture object.
			/// \param[in] clear_col the clear value to assign to this render texture.
			/// \param[in] descr the attachment description that will be used to construct the core::data::framebuffer::attachment::description.
			/// \param[in] shared value indicating if this render attachment is shared within this framebuffer (see core::data::framebuffer::attachment::shared() for more info).
			attachment(const psl::UID& texture, const vk::ClearValue& clear_col, vk::AttachmentDescription descr, bool shared = false);

			/// \brief returns the psl::UID assigned to this render attachment.
			/// \returns the psl::UID assigned to this render attachment.
			/// \note the texture resource assigned to the UID is not guaranteed to be loaded (or even constructed), as it can have just been parsed from disk.
			const psl::UID& texture() const;

			/// \brief returns the clear value assigned to this attachment.
			/// \returns the clear value assigned to this attachment.
			const vk::ClearValue& clear_value() const;

			/// \brief returns a vk::AttachmentDescription based on the internal settings. This is a passthrough method to the core::data::framebuffer::attachment::description instance.
			/// \returns a vk::AttachmentDescription based on the internal settings.
			vk::AttachmentDescription vkDescription() const;

			/// \brief signifies if this specific attachment duplicated when the framebuffer's image count is larger than 1.
			///
			/// Sometimes you don't need a render attachment to have a unique instance per framebuffer entry (for example depth testing/texture in a double buffer scenario).
			/// in this case you can set this render attachment to be "shared", a flag that will tell the implementation that rather than creating a new instance for when the
			/// framebuffer count is larger than 1, it should instead reuse the current one.
			/// \see core::gfx::framebuffer for the application of this flag.
			/// \returns true if this attachment is duplicated (true) or not (false).
			bool shared() const;
		private:
			template <typename S>
			void serialize(S& serializer)
			{
				serializer << m_Texture << m_ClearValue << m_Shared;
			}

			psl::serialization::property<psl::UID, const_str("TEXTURE UID", 11)> m_Texture;
			psl::serialization::property<vk::ClearValue, const_str("CLEAR VALUE", 11)> m_ClearValue;
			psl::serialization::property<description, const_str("DESCRIPTION", 11)> m_Description;
			psl::serialization::property<bool, const_str("SHARED", 6)> m_Shared;
		};

		/// \brief basic constructor that sets up the rough outlines of an instance
		/// \note you will still need to set up attachments, etc.. later on. The constructor makes a "valid" instance in the sense
		/// that manipulating it will not cause undefined behaviour, but you cannot create a core::gfx::framebuffer just yet with this
		/// after calling the constructor.
		/// \param[in] uid the resouce system assigned psl::UID.
		/// \param[in] cache signifies in which cache I will be constructed in.
		/// \param[in] width the width in pixels of this framebuffer.
		/// \param[in] height the height in pixels of this framebuffer.
		/// \param[in] layers the amount of layers this framebuffer will have (often referred to as the framebuffer count in the documentation).
		framebuffer(const psl::UID& uid, core::resource::cache& cache, uint32_t width, uint32_t height, uint32_t layers = 1u);
		framebuffer(const framebuffer& other, const psl::UID& uid, core::resource::cache& cache);

		/// \brief adds a core::data::framebuffer::attachment to the current framebuffer.
		/// \param[in] width the width of the attachment in pixels.
		/// \param[in] height the height of the attachment in pixels.
		/// \param[in] layerCount the depth layers of the attachment.
		/// \param[in] usage signifies how the image will be used.
		/// \param[in] clearValue the value to clear the image with at the start of rendering.
		/// \param[in] descr how the image load op's etc... will be handled.
		/// \see core::data::framebuffer::attachment
		const psl::UID& add(uint32_t width, uint32_t height, uint32_t layerCount, vk::ImageUsageFlags usage, vk::ClearValue clearValue, vk::AttachmentDescription descr);

		/// \brief removes the attachment that is using this psl::UID for its texture.
		/// \param[in] uid the psl::UID to search for
		/// \returns true in case it found and removed atleast one attachment with the given UID.
		bool remove(const psl::UID& uid);

		/// \brief sets the sampler associated with this framebuffer
		/// \param[in] sampler a valid sampler resource
		/// \note the sampler does not need to be loaded, we will only store the psl::UID, but if the framebuffer is used for rendering, and
		/// the sampler is invalid, then what follows is undefined behaviour.
		void set(core::resource::handle<core::gfx::sampler> sampler);

		/// \brief gets all attachments currently assigned to this framebuffer.
		/// \returns all attachments currently assigned to this framebuffer.
		const std::vector<attachment>& attachments() const;

		/// \brief returns the framebuffer count (layers).
		/// \returns the framebuffer count (layers).
		uint32_t framebuffers() const;

		/// \brief returns the sampler associated with this framebuffer (if it has been set).
		/// \returns the sampler associated with this framebuffer (if it has been set).
		std::optional<psl::UID> sampler() const;

		/// \brief returns the width of the framebuffer object.
		uint32_t width() const;
		/// \brief returns the height of the framebuffer object.
		uint32_t height() const;
	private:
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_Width << m_Height << m_Count << m_Sampler << m_Attachments;
		}
		static constexpr const char serialization_name[12]{"FRAMEBUFFER"};

		psl::serialization::property<std::vector<attachment>, const_str("ATTACHMENTS", 11)> m_Attachments;
		psl::serialization::property<psl::UID, const_str("SAMPLER", 7)> m_Sampler;
		psl::serialization::property<uint32_t, const_str("WIDTH", 5)> m_Width;
		psl::serialization::property<uint32_t, const_str("HEIGHT", 6)> m_Height;
		psl::serialization::property<uint32_t, const_str("FRAMEBUFFER COUNT", 17)> m_Count;
		core::resource::cache& m_Cache;
	};
}
