#pragma once
#include "stdafx.h"

namespace core::data
{
	/// \brief describes the data to build a core::gfx::sampler instance
	class sampler final
	{
		friend class psl::serialization::accessor;
	public:
		sampler(const psl::UID& uid, core::resource::cache& cache);
		~sampler() = default;
		sampler(const sampler& other) = delete;
		sampler(sampler&& other) = delete;
		sampler& operator=(const sampler& other) = delete;
		sampler& operator=(sampler&& other) = delete;

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
		void  mip_bias(float value);

		/// \brief returns the mode for mipmap texture lookups.
		/// \returns the mode for mipmap texture lookups.
		vk::SamplerMipmapMode mip_mode() const;

		/// \brief sets the mode for mipmap texture lookups.
		/// \param[in] value the mode to change to.
		void mip_mode(vk::SamplerMipmapMode value);

		/// \brief returns the minimal mipmap LOD this instance has set.
		/// \returns the minimal mipmap LOD this instance has set.
		float mip_minlod() const;

		/// \brief sets the minimal mipmap LOD offset for this instance.
		/// \param[in] value the value to set.
		void mip_minlod(float value);

		/// \brief returns the max mipmap LOD this instance has set.
		/// \returns the max mipmap LOD this instance has set.
		/// \todo this value is currently ignored in core::gfx::sampler.
		float mip_maxlod() const;
		/// \brief sets the minimal mipmap max LOD for this instance.
		/// \param[in] value the value to set.
		void mip_maxlod(float value);

		/// \brief returns how this instances deals with texture tiling in the U-axis.
		/// \returns how this instances deals with texture tiling in the U-axis.
		vk::SamplerAddressMode addressU() const;
		/// \brief sets how this instance should deal with texture tiling in the U-axis.
		/// \param[in] value the mode to set this instance to.
		void addressU(vk::SamplerAddressMode value);

		/// \brief returns how this instances deals with texture tiling in the V-axis.
		/// \returns how this instances deals with texture tiling in the V-axis.
		vk::SamplerAddressMode addressV() const;
		/// \brief sets how this instance should deal with texture tiling in the V-axis.
		/// \param[in] value the mode to set this instance to.
		void addressV(vk::SamplerAddressMode value);

		/// \brief returns how this instances deals with texture tiling in the W-axis.
		/// \returns how this instances deals with texture tiling in the W-axis.
		vk::SamplerAddressMode addressW() const;
		/// \brief sets how this instance should deal with texture tiling in the W-axis.
		/// \param[in] value the mode to set this instance to.
		void addressW(vk::SamplerAddressMode value);

		/// \brief returns the border color that will be used during texture lookups.
		/// \returns the border color that will be used during texture lookups.
		vk::BorderColor border_color() const;

		/// \brief sets the border color that should be used during texture lookups.
		/// \param[in] value the new border color value.
		void border_color(vk::BorderColor value);

		/// \brief returns if anisotropic filtering is enabled.
		/// \returns if anisotropic filtering is enabled.
		bool anisotropic_filtering() const;

		/// \brief call this to enable or disable anisotropic filtering.
		/// \param[in] value set to true to enable anisotropic filtering.
		/// \note if the current core::gfx::context doesn't support anisotropic filtering, 
		/// then this value will be ingored upstream (core::gfx::sampler).
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
		vk::CompareOp compare_op() const;
		/// \brief sets the compare op to a new value.
		/// \param[in] value the new compare op to use.
		/// \note compare_op() will only be used if compare_mode() is true. You can still set this value regardless however.
		void compare_op(vk::CompareOp value);

		/// \brief returns the filtering mode to use when dealing with minification.
		/// \returns the filtering mode to use when dealing with minification.
		vk::Filter filter_min() const;
		
		/// \brief sets the filtering mode to use when dealing with minification.
		/// \param[in] value the new filter mode to use.
		void filter_min(vk::Filter value);

		/// \brief returns the filtering mode to use when dealing with magnification.
		/// \returns the filtering mode to use when dealing with magnification.
		vk::Filter filter_max() const;
		/// \brief sets the filtering mode to use when dealing with magnification.
		/// \param[in] value the new filter mode to use.
		void filter_max(vk::Filter value);

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
			if(m_MipMapped.value)
				serializer << m_MipMapMode << m_MipLodBias << m_MinLod << m_MaxLod;
			serializer << m_AddressModeU << m_AddressModeV << m_AddressModeW << m_BorderColor << m_AnisotropyEnable;
			if(m_AnisotropyEnable.value)
				serializer << m_MaxAnisotropy;
			serializer << m_CompareEnable;
			if(m_CompareEnable.value)
				serializer << m_CompareOp;
			serializer << m_MinFilter << m_MaxFilter << m_NormalizedCoordinates;
		}

		static constexpr const char serialization_name[8]{"SAMPLER"};

		psl::serialization::property<bool, const_str("MIPMAPS", 7)>						m_MipMapped = true;
		psl::serialization::property<vk::SamplerMipmapMode, const_str("MIP_MODE", 8)>	m_MipMapMode = vk::SamplerMipmapMode::eNearest;
		psl::serialization::property<float, const_str("MIP_BIAS", 8)>					m_MipLodBias = 0.0f;
		psl::serialization::property<float, const_str("MIP_MIN", 7)>						m_MinLod = 0.0f;
		psl::serialization::property<float, const_str("MIP_MAX", 7)>						m_MaxLod = 0.0f;

		psl::serialization::property<vk::SamplerAddressMode, const_str("ADDRESS_U",9)>	m_AddressModeU = vk::SamplerAddressMode::eRepeat;
		psl::serialization::property<vk::SamplerAddressMode, const_str("ADDRESS_V", 9)>	m_AddressModeV = vk::SamplerAddressMode::eRepeat;
		psl::serialization::property<vk::SamplerAddressMode, const_str("ADDRESS_W", 9)>	m_AddressModeW = vk::SamplerAddressMode::eRepeat;
		psl::serialization::property<vk::BorderColor, const_str("BORDER_COLOR", 12)>		m_BorderColor = vk::BorderColor::eFloatTransparentBlack;

		psl::serialization::property<bool, const_str("ANISOTROPY",10)>					m_AnisotropyEnable = true;
		psl::serialization::property<float, const_str("MAX_ANISO", 9)>					m_MaxAnisotropy = 2.0f;

		psl::serialization::property<bool, const_str("COMPARE", 7)>						m_CompareEnable = false;
		psl::serialization::property<vk::CompareOp, const_str("COMPARE_OPERATION", 17)>	m_CompareOp = vk::CompareOp::eNever;

		psl::serialization::property<vk::Filter, const_str("FILTER_MIN", 10)>			m_MinFilter = vk::Filter::eLinear;
		psl::serialization::property<vk::Filter, const_str("FILTER_MAX", 10)>			m_MaxFilter = vk::Filter::eLinear;

		psl::serialization::property<bool, const_str("NORMALIZED_COORDINATES",22)>		m_NormalizedCoordinates = true;
	};
}
