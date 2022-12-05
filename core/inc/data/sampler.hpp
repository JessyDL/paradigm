#pragma once
#include "fwd/resource/resource.hpp"
#include "gfx/types.hpp"
#include "psl/serialization/serializer.hpp"


namespace core::data
{
	/// \brief describes the data to build a core::ivk::sampler_t instance
	class sampler_t final
	{
		friend class psl::serialization::accessor;

	  public:
		sampler_t(core::resource::cache_t& cache,
				const core::resource::metadata& metaData,
				psl::meta::file* metaFile) noexcept;
		~sampler_t()					  = default;
		sampler_t(const sampler_t& other) = delete;
		sampler_t(sampler_t&& other)	  = delete;
		sampler_t& operator=(const sampler_t& other) = delete;
		sampler_t& operator=(sampler_t&& other) = delete;

		/// \brief returns if mipmapping is enabled or not
		/// \returns if mipmapping is enabled or not
		bool mipmaps() const;
		/// \brief enables or disables mipmapping on this sampler.
		/// \param[in] value true for enabling mipmapping.
		void mipmaps(bool value);

		/// \brief returns the mip_bias that might be present on this sampler.
		/// \returns the mip_bias that might be present on this sampler.
		/// \note mipmaps() should be true for this to have an effect.
		float mip_bias() const;
		/// \brief sets the mip bias when sampling mipmaps.
		/// \param[in] value the bias to apply to mipmap sampling.
		/// \note mipmaps() should be true for this to have an effect.
		void mip_bias(float value);

		/// \brief returns the mode for mipmap texture lookups.
		/// \returns the mode for mipmap texture lookups.
		core::gfx::sampler_mipmap_mode mip_mode() const;

		/// \brief sets the mode for mipmap texture lookups.
		/// \param[in] value the mode to change to.
		void mip_mode(core::gfx::sampler_mipmap_mode value);

		/// \brief returns the minimal mipmap LOD this instance has set.
		/// \returns the minimal mipmap LOD this instance has set.
		float mip_minlod() const;

		/// \brief sets the minimal mipmap LOD offset for this instance.
		/// \param[in] value the value to set.
		void mip_minlod(float value);

		/// \brief returns the max mipmap LOD this instance has set.
		/// \returns the max mipmap LOD this instance has set.
		/// \todo this value is currently ignored in core::core::ivk::sampler_t.
		float mip_maxlod() const;
		/// \brief sets the minimal mipmap max LOD for this instance.
		/// \param[in] value the value to set.
		void mip_maxlod(float value);

		/// \brief returns how this instances deals with texture tiling in the U-axis.
		/// \returns how this instances deals with texture tiling in the U-axis.
		core::gfx::sampler_address_mode addressU() const;
		/// \brief sets how this instance should deal with texture tiling in the U-axis.
		/// \param[in] value the mode to set this instance to.
		void addressU(core::gfx::sampler_address_mode value);

		/// \brief returns how this instances deals with texture tiling in the V-axis.
		/// \returns how this instances deals with texture tiling in the V-axis.
		core::gfx::sampler_address_mode addressV() const;
		/// \brief sets how this instance should deal with texture tiling in the V-axis.
		/// \param[in] value the mode to set this instance to.
		void addressV(core::gfx::sampler_address_mode value);

		/// \brief returns how this instances deals with texture tiling in the W-axis.
		/// \returns how this instances deals with texture tiling in the W-axis.
		core::gfx::sampler_address_mode addressW() const;
		/// \brief sets how this instance should deal with texture tiling in the W-axis.
		/// \param[in] value the mode to set this instance to.
		void addressW(core::gfx::sampler_address_mode value);

		void address(core::gfx::sampler_address_mode value) noexcept;
		void address(core::gfx::sampler_address_mode u,
					 core::gfx::sampler_address_mode v,
					 core::gfx::sampler_address_mode w) noexcept;
		/// \brief returns the border color that will be used during texture lookups.
		/// \returns the border color that will be used during texture lookups.
		core::gfx::border_color border_color() const;

		/// \brief sets the border color that should be used during texture lookups.
		/// \param[in] value the new border color value.
		void border_color(core::gfx::border_color value);

		/// \brief returns if anisotropic filtering is enabled.
		/// \returns if anisotropic filtering is enabled.
		bool anisotropic_filtering() const;

		/// \brief call this to enable or disable anisotropic filtering.
		/// \param[in] value set to true to enable anisotropic filtering.
		/// \note if the current core::ivk::context doesn't support anisotropic filtering,
		/// then this value will be ingored upstream (core::core::ivk::sampler_t).
		void anisotropic_filtering(bool value);

		/// \brief returns the max anistropic value for this instance.
		/// \returns the max anistropic value for this instance.
		float max_anisotropy() const;
		/// \brief sets the max anistropic value for this instance.
		/// \param[in] value the value to set the max anisotropic value.
		void max_anisotropy(float value);

		/// \brief returns if compare operations have been enabled or not.
		/// \returns if compare operations have been enabled or not.
		bool compare_mode() const;
		/// \brief enables or disables compare ops.
		/// \param[in] value set to true to enable compare ops.
		void compare_mode(bool value);

		/// \brief returns what compare op would be used if compare_mode() is true.
		/// \returns what compare op would be used if compare_mode() is true.
		core::gfx::compare_op compare_op() const;
		/// \brief sets the compare op to a new value.
		/// \param[in] value the new compare op to use.
		/// \note compare_op() will only be used if compare_mode() is true. You can still set this value regardless
		/// however.
		void compare_op(core::gfx::compare_op value);

		/// \brief returns the filtering mode to use when dealing with minification.
		/// \returns the filtering mode to use when dealing with minification.
		core::gfx::filter filter_min() const;

		/// \brief sets the filtering mode to use when dealing with minification.
		/// \param[in] value the new filter mode to use.
		void filter_min(core::gfx::filter value);

		/// \brief returns the filtering mode to use when dealing with magnification.
		/// \returns the filtering mode to use when dealing with magnification.
		core::gfx::filter filter_max() const;
		/// \brief sets the filtering mode to use when dealing with magnification.
		/// \param[in] value the new filter mode to use.
		void filter_max(core::gfx::filter value);

		/// \brief returns if the coordinates for this sampler will be normalized or not.
		/// \returns if the coordinates for this sampler will be normalized or not.
		bool normalized_coordinates() const;
		/// \brief enables or disables coordinate normalization when this sampler is used.
		/// \param[in] value enables or disables the behaviour.
		void normalized_coordinates(bool value);

	  private:
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_MipMapped;
			if(m_MipMapped.value) serializer << m_MipMapMode << m_MipLodBias << m_MinLod << m_MaxLod;
			serializer << m_AddressModeU << m_AddressModeV << m_AddressModeW << m_BorderColor << m_AnisotropyEnable;
			if(m_AnisotropyEnable.value) serializer << m_MaxAnisotropy;
			serializer << m_CompareEnable;
			if(m_CompareEnable.value) serializer << m_CompareOp;
			serializer << m_MinFilter << m_MaxFilter << m_NormalizedCoordinates;
		}

		static constexpr psl::string8::view serialization_name {"SAMPLER"};

		psl::serialization::property<"MIPMAPS", bool> m_MipMapped = true;
		psl::serialization::property<"MIP_MODE", core::gfx::sampler_mipmap_mode> m_MipMapMode =
		  core::gfx::sampler_mipmap_mode::nearest;
		psl::serialization::property<"MIP_BIAS", float> m_MipLodBias = 0.0f;
		psl::serialization::property<"MIP_MIN", float> m_MinLod		 = 0.0f;
		psl::serialization::property<"MIP_MAX", float> m_MaxLod		 = 14.0f;

		psl::serialization::property<"ADDRESS_U", core::gfx::sampler_address_mode> m_AddressModeU =
		  core::gfx::sampler_address_mode::repeat;
		psl::serialization::property<"ADDRESS_V", core::gfx::sampler_address_mode> m_AddressModeV =
		  core::gfx::sampler_address_mode::repeat;
		psl::serialization::property<"ADDRESS_W", core::gfx::sampler_address_mode> m_AddressModeW =
		  core::gfx::sampler_address_mode::repeat;
		psl::serialization::property<"BORDER_COLOR", core::gfx::border_color> m_BorderColor =
		  core::gfx::border_color::float_transparent_black;

		psl::serialization::property<"ANISOTROPY", bool> m_AnisotropyEnable = true;
		psl::serialization::property<"MAX_ANISO", float> m_MaxAnisotropy	= 8.0f;

		psl::serialization::property<"COMPARE", bool> m_CompareEnable = false;
		psl::serialization::property<"COMPARE_OPERATION", core::gfx::compare_op> m_CompareOp =
		  core::gfx::compare_op::never;

		psl::serialization::property<"FILTER_MIN", core::gfx::filter> m_MinFilter = core::gfx::filter::linear;
		psl::serialization::property<"FILTER_MAX", core::gfx::filter> m_MaxFilter = core::gfx::filter::linear;

		psl::serialization::property<"NORMALIZED_COORDINATES", bool> m_NormalizedCoordinates = true;
	};
}	 // namespace core::data
