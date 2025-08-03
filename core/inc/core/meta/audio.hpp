#pragma once
#include "core/audio/audio.hpp"
#include "psl/library.hpp"
#include "psl/serialization/serializer.hpp"
#include "psl/ustring.hpp"

namespace core::meta {
class audio_t final : public psl::meta::file {
  public:
	friend class psl::serialization::accessor;
	friend class core::audio::audio_t;

	template <psl::details::fixed_astring Name, typename T>
	using property = psl::serialization::property<Name, T>;

	audio_t() {};
	explicit audio_t(const psl::UID& key) noexcept : psl::meta::file(key) {};
	~audio_t() = default;

	auto format() const noexcept -> core::audio::format_t {
		return m_Format.value;
	}
	auto volume() const noexcept -> float {
		return m_Volume.value;
	}
	auto pitch() const noexcept -> float {
		return m_Pitch.value;
	}
	auto loop() const noexcept -> bool {
		return m_Loop.value;
	}
	auto start_offset() const noexcept -> float {
		return m_StartOffset.value;
	}
	auto end_offset() const noexcept -> float {
		return m_EndOffset.value;
	}
	auto attenuation() const noexcept -> float {
		return m_Attenuation.value;
	}
	auto fade_in_duration() const noexcept -> float {
		return m_FadeInDuration.value;
	}
	auto fade_out_duration() const noexcept -> float {
		return m_FadeOutDuration.value;
	}
	auto spatial() const noexcept -> bool {
		return m_Spatial.value;
	}
	auto spatial_radius() const noexcept -> float {
		return m_SpatialRadius.value;
	}

	void format(core::audio::format_t value) noexcept {
		m_Format = value;
	}
	void volume(float value) noexcept {
		m_Volume = value;
	}
	void pitch(float value) noexcept {
		m_Pitch = value;
	}
	void loop(bool value) noexcept {
		m_Loop = value;
	}
	void start_offset(float value) noexcept {
		m_StartOffset = value;
	}
	void end_offset(float value) noexcept {
		m_EndOffset = value;
	}
	void attenuation(float value) noexcept {
		m_Attenuation = value;
	}
	void fade_in_duration(float value) noexcept {
		m_FadeInDuration = value;
	}
	void fade_out_duration(float value) noexcept {
		m_FadeOutDuration = value;
	}
	void spatial(bool value) noexcept {
		m_Spatial = value;
	}
	void spatial_radius(float value) noexcept {
		m_SpatialRadius = value;
	}

  private:
	template <typename S>
	void serialize(S& s) {
		psl::meta::file::serialize(s);

		s << m_Format << m_Volume << m_Pitch << m_Loop << m_StartOffset << m_EndOffset << m_Attenuation
		  << m_FadeInDuration << m_FadeOutDuration << m_Spatial << m_SpatialRadius;
	}

	property<"FORMAT", core::audio::format_t> m_Format {core::audio::format_t::unknown};
	property<"VOLUME", float> m_Volume {1.0f};
	property<"PITCH", float> m_Pitch {1.0f};
	property<"LOOP", bool> m_Loop {false};
	property<"START_OFFSET", float> m_StartOffset {0.0f};
	property<"END_OFFSET", float> m_EndOffset {0.0f};
	property<"ATTENUATION", float> m_Attenuation {1.0f};
	property<"FADE_IN_DURATION", float> m_FadeInDuration {0.0f};
	property<"FADE_OUT_DURATION", float> m_FadeOutDuration {0.0f};
	property<"SPATIAL_RADIUS", float> m_SpatialRadius {1.0f};
	property<"SPATIAL", bool> m_Spatial {false};

	/// \brief the polymorphic serialization name for the psl::format::node that will be used to calculate the CRC64
	/// ID of this type on.
	static constexpr psl::string8::view polymorphic_name {"AUDIO_META"};
	/// \brief returns the polymorphic ID at runtime, to resolve what type this is.
	virtual const uint64_t polymorphic_id() override {
		return polymorphic_identity;
	}
	/// \brief the associated unique ID (per type, not instance) for the polymorphic system.
	volatile static const uint64_t polymorphic_identity;
};
}	 // namespace core::meta
