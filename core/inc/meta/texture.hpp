﻿#pragma once
#include "gfx/types.hpp"
#include "psl/library.hpp"
#include "psl/meta.hpp"
#include "psl/serialization/serializer.hpp"

namespace core::resource
{
	class cache_t;
}

namespace core::meta
{
	/// \brief Custom meta data that describes a texture. i.e. width, height, format, etc..
	///
	/// does not contain any actual texture data, just describes it. This aids the engine in setting up
	/// the correct resources for rendering the associated texture file.
	/// \warning The description is assumed to be correct, otherwise undefined behaviour happens.
	/// \note although this class allows runtime editing, it's unlikely you'd need it unless you generate textures.
	/// and wish to serialize them to disk.
	class texture_t final : public psl::meta::file
	{
		friend class psl::serialization::accessor;
	  public:
		texture_t() = default;
		explicit texture_t(const psl::UID& key) noexcept : psl::meta::file(key) {};

		~texture_t() = default;

		/// \returns the width of the texture in pixels.
		uint32_t width() const noexcept;
		/// \brief sets the width in pixels of the given texture.
		/// \note even though setting this to less than the actual resolution should be fine, setting it to more will
		/// result in accessing outside of the actual memory leading to a crash.
		/// \param[in] width the width in pixels
		void width(uint32_t width);

		/// \returns the height of the texture in pixels.
		uint32_t height() const noexcept;
		/// \brief sets the height in pixels of the given texture.
		/// \note even though setting this to less than the actual resolution should be fine, setting it to more will
		/// result in accessing outside of the actual memory leading to a crash.
		/// \param[in] height the height in pixels.
		void height(uint32_t height);

		/// \returns the depth of the texture in pixels.
		uint32_t depth() const noexcept;
		/// \brief sets the depth in pixels of the given texture.
		/// \note even though setting this to less than the actual resolution should be fine, setting it to more will
		/// result in accessing outside of the actual memory leading to a crash.
		/// \param[in] depth the depth in pixels.
		void depth(uint32_t depth);

		/// \returns the used mip levels of the texture.
		uint32_t mip_levels() const noexcept;
		/// \brief sets the used mip levels of the texture.
		/// \warning although setting less than the actual available mip_levels is possible (but leads to undefined
		/// behaviour), setting more than the available will definitely lead to a crash.
		/// \param[in] mip_levels the amount of mip levels present in the resource.
		void mip_levels(uint32_t mip_levels);

		/// \returns the layers of the texture.
		uint32_t layers() const noexcept;
		/// \brief sets the amount of layers this texture exists out of (think array textures).
		/// \param[in] layers the amount of layers contained in this instance.
		/// \note even though setting this to less than the actual layers should be fine, setting it to more will result
		/// in accessing outside of the actual memory leading to a crash.
		void layers(uint32_t layers);

		/// \returns the format that the texture is in.
		core::gfx::format_t format() const noexcept;
		/// \brief the format of the texture resource.
		/// \param[in] format the expected format.
		/// \warning invalid formats will, in best case lead to render artifacts, and in worst cause segmentation fault
		/// for reading outside of the memory that was expected to be present for the given format.
		void format(core::gfx::format_t format);

		/// \returns the type of texture this is (2D, 3D, etc..).
		core::gfx::image_type image_type() const noexcept;
		/// \brief sets the type of texture (2D, 3D, 2DArray, etc...).
		/// \param[in] type the type of image to expect.
		void image_type(core::gfx::image_type type);

		/// \returns the expected usage of this texture
		/// \note this can be changed, it is a suggestion.
		core::gfx::image_usage usage() const noexcept;
		/// \brief suggests the usage flags of this texture.
		/// \param[in] usage the usage flags to expect.
		void usage(core::gfx::image_usage usage);

		/// \returns what type of aspects to expect (color, depth, stencil, etc..)
		/// \note this can be changed, it is a suggestion.
		core::gfx::image_aspect aspect_mask() const noexcept;
		/// \brief suggests the aspect masks this texture exists out of.
		/// \note invalid aspect masks lead to undefined behaviour.
		/// \param[in] aspect the aspect mask flag to expect.
		void aspect_mask(core::gfx::image_aspect aspect);

	  private:
		/// \brief method that will be invoked by the serialization system.
		/// \tparam S the type of the serializer/deserializer
		/// \param[in] s instance of a serializer that you can read from, or write to.
		template <typename S>
		void serialize(S& s)
		{
			psl::meta::file::serialize(s);

			s << m_Width << m_Height << m_Depth << m_MipLevels << m_LayerCount << m_Format << m_ImageType
			  << m_UsageFlags << m_AspectMask;

			assert((uint32_t)m_Format.value <= 97 || (uint32_t)m_Format.value >= 120);
		}
		/// \brief validates this texture
		bool validate() const noexcept;
		psl::serialization::property<"WIDTH", uint32_t> m_Width {0u};
		psl::serialization::property<"HEIGHT", uint32_t> m_Height {0u};
		psl::serialization::property<"DEPTH", uint32_t> m_Depth {1u};
		psl::serialization::property<"MIP_LEVELS", uint32_t> m_MipLevels {1u};
		psl::serialization::property<"LAYERS", uint32_t> m_LayerCount {1u};
		psl::serialization::property<"FORMAT", core::gfx::format_t> m_Format {core::gfx::format_t::undefined};
		psl::serialization::property<"IMAGE_TYPE", core::gfx::image_type> m_ImageType {
		  core::gfx::image_type::planar_2D};
		psl::serialization::property<"USAGE", core::gfx::image_usage> m_UsageFlags {
		  core::gfx::image_usage::transfer_destination | core::gfx::image_usage::sampled};
		psl::serialization::property<"ASPECT_MASK", core::gfx::image_aspect> m_AspectMask {
		  core::gfx::image_aspect::color};


		/// \brief the polymorphic serialization name for the psl::format::node that will be used to calculate the CRC64
		/// ID of this type on.
		static constexpr psl::string8::view polymorphic_name {"TEXTURE_META"};
		/// \brief returns the polymorphic ID at runtime, to resolve what type this is.
		virtual const uint64_t polymorphic_id() override { return polymorphic_identity; }
		/// \brief the associated unique ID (per type, not instance) for the polymorphic system.
		static const uint64_t polymorphic_identity;
	};
}	 // namespace core::meta
