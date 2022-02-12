#pragma once
#include "fwd/resource/resource.hpp"
#include "gfx/types.hpp"
#include "psl/meta.hpp"
#include "psl/serialization/serializer.hpp"

namespace core::gfx
{
	class sampler_t;
}

namespace core::data
{
	/// \brief container class that describes the data to create a set of rendertargets.
	class framebuffer_t final
	{
		friend class psl::serialization::accessor;

	  public:
		/// \brief describes a single rendertarget in a framebuffer.
		///
		/// all data contained within this object is not guaranteed to be loaded, this includes the psl::UID of the
		/// core::gfx::texture_t. You should take caution when calling methods that manipulate the contained resources
		/// for this reason.
		class attachment final
		{
			friend class psl::serialization::accessor;
			friend class framebuffer_t;

		  public:
			/// \brief describes how the rendertarget should be loaded, stored, it's sample count, and what transition
			/// layouts it should go through.
			class description final
			{
				friend class psl::serialization::accessor;

			  public:
				/// \brief defaulted constructor
				description() = default;
				/// \brief construction based on a core::gfx::attachment.
				description(core::gfx::attachment descr) noexcept;
				/// \brief returns a core::gfx::attachment based on the internal settings.
				/// \returns a core::gfx::attachment based on the internal settings.
				operator core::gfx::attachment() const noexcept;

			  private:
				template <typename S>
				void serialize(S& serializer)
				{
					serializer << m_SampleCountFlags << m_LoadOp << m_StoreOp << m_StencilLoadOp << m_StencilStoreOp
							   << m_InitialLayout << m_FinalLayout << m_Format;
				}
				static constexpr psl::string8::view serialization_name {"DESCRIPTION"};
				psl::serialization::property<"SAMPLE_COUNT_BITS", uint8_t> m_SampleCountFlags;
				psl::serialization::property<"LOAD_OP", core::gfx::attachment::load_op> m_LoadOp;
				psl::serialization::property<"STORE_OP", core::gfx::attachment::store_op> m_StoreOp;
				psl::serialization::property<"STENCIl_LOAD_OP", core::gfx::attachment::load_op> m_StencilLoadOp;
				psl::serialization::property<"STENCIl_STORE_OP", core::gfx::attachment::store_op> m_StencilStoreOp;
				psl::serialization::property<"INITIAL_LAYOUT", core::gfx::image::layout> m_InitialLayout;
				psl::serialization::property<"FINAL_LAYOUT", core::gfx::image::layout> m_FinalLayout;
				psl::serialization::property<"FORMAT", core::gfx::format_t> m_Format;
			};

			attachment() = default;

			/// \brief constructs an attachment based on the given texture and values.
			///
			/// \note that the texture should be valid (not pointing to a non-texture),
			/// otherwise you might run into issues further down.
			/// \param[in] texture a psl::UID pointing to a valid (known or constructed), core::ivk::texture_t object.
			/// \param[in] clear_col the clear value to assign to this render texture.
			/// \param[in] descr the attachment description that will be used to construct the
			/// core::data::framebuffer_t::attachment::description.
			/// \param[in] shared value indicating if this render attachment is shared within this framebuffer (see
			/// core::data::framebuffer_t::attachment::shared() for more info).
			attachment(const psl::UID& texture,
					   const core::gfx::clear_value& clear_col,
					   core::gfx::attachment descr,
					   bool shared = false);

			/// \brief returns the psl::UID assigned to this render attachment.
			/// \returns the psl::UID assigned to this render attachment.
			/// \note the texture resource assigned to the UID is not guaranteed to be loaded (or even constructed), as
			/// it can have just been parsed from disk.
			const psl::UID& texture() const;

			/// \brief returns the clear value assigned to this attachment.
			/// \returns the clear value assigned to this attachment.
			const core::gfx::clear_value& clear_value() const;

			/// \brief returns a core::gfx::attachment based on the internal settings. This is a passthrough method to
			/// the core::data::framebuffer_t::attachment::description instance.
			/// \returns a core::gfx::attachment based on the internal settings.
			operator core::gfx::attachment() const noexcept;

			/// \brief signifies if this specific attachment duplicated when the framebuffer's image count is larger
			/// than 1.
			///
			/// Sometimes you don't need a render attachment to have a unique instance per framebuffer entry (for
			/// example depth testing/texture in a double buffer scenario). in this case you can set this render
			/// attachment to be "shared", a flag that will tell the implementation that rather than creating a new
			/// instance for when the framebuffer count is larger than 1, it should instead reuse the current one.
			/// \see core::ivk::framebuffer_t for the application of this flag.
			/// \returns true if this attachment is duplicated (true) or not (false).
			bool shared() const;

		  private:
			template <typename S>
			void serialize(S& serializer)
			{
				serializer << m_Texture /*<< m_ClearValue*/ << m_Shared;
			}

			psl::serialization::property<"TEXTURE UID", psl::UID> m_Texture;
			psl::serialization::property<"CLEAR VALUE", core::gfx::clear_value> m_ClearValue;
			psl::serialization::property<"DESCRIPTION", description> m_Description;
			psl::serialization::property<"SHARED", bool> m_Shared;
		};

		/// \brief basic constructor that sets up the rough outlines of an instance
		/// \note you will still need to set up attachments, etc.. later on. The constructor makes a "valid" instance in
		/// the sense that manipulating it will not cause undefined behaviour, but you cannot create a
		/// core::ivk::framebuffer_t just yet with this after calling the constructor.
		/// \param[in] cache signifies in which cache I will be constructed in.
		/// \param[in] metaData the metadata that is assigned to this object
		/// \param[in] metaFile the metafile associated with this instance
		/// \param[in] width the width in pixels of this framebuffer.
		/// \param[in] height the height in pixels of this framebuffer.
		/// \param[in] layers the amount of layers this framebuffer will have (often referred to as the framebuffer
		/// count in the documentation).
		framebuffer_t(core::resource::cache_t& cache,
					  const core::resource::metadata& metaData,
					  psl::meta::file* metaFile,
					  uint32_t width,
					  uint32_t height,
					  uint32_t layers = 1u) noexcept;
		// framebuffer_t(const framebuffer_t& other, const psl::UID& uid, core::resource::cache_t& cache);

		/// \brief adds a core::data::framebuffer_t::attachment to the current framebuffer.
		/// \param[in] width the width of the attachment in pixels.
		/// \param[in] height the height of the attachment in pixels.
		/// \param[in] layerCount the depth layers of the attachment.
		/// \param[in] usage signifies how the image will be used.
		/// \param[in] clearValue the value to clear the image with at the start of rendering.
		/// \param[in] descr how the image load op's etc... will be handled.
		/// \see core::data::framebuffer_t::attachment
		const psl::UID& add(uint32_t width,
							uint32_t height,
							uint32_t layerCount,
							core::gfx::image::usage usage,
							core::gfx::clear_value clearValue,
							core::gfx::attachment descr);

		/// \brief removes the attachment that is using this psl::UID for its texture.
		/// \param[in] uid the psl::UID to search for
		/// \returns true in case it found and removed atleast one attachment with the given UID.
		bool remove(const psl::UID& uid);

		/// \brief sets the sampler associated with this framebuffer
		/// \param[in] sampler a valid sampler resource
		/// \note the sampler does not need to be loaded, we will only store the psl::UID, but if the framebuffer is
		/// used for rendering, and the sampler is invalid, then what follows is undefined behaviour.
		void set(core::resource::handle<core::gfx::sampler_t> sampler);

		/// \brief gets all attachments currently assigned to this framebuffer.
		/// \returns all attachments currently assigned to this framebuffer.
		const std::vector<attachment>& attachments() const { return m_Attachments.value; };

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
		static constexpr psl::string8::view serialization_name {"FRAMEBUFFER"};

		psl::serialization::property<"ATTACHMENTS", std::vector<attachment>> m_Attachments;
		psl::serialization::property<"SAMPLER", psl::UID> m_Sampler;
		psl::serialization::property<"WIDTH", uint32_t> m_Width;
		psl::serialization::property<"HEIGHT", uint32_t> m_Height;
		psl::serialization::property<"FRAMEBUFFER COUNT", uint32_t> m_Count;
		core::resource::cache_t* m_Cache;
	};
}	 // namespace core::data
