#pragma once
#include "core/fwd/audio/audio.hpp"

namespace core::audio {
enum class format_t : uint8_t { unknown = 0, mp3 = 1, vorbis = 2, pcm = 3, wav = 4, flac = 5 };
class audio_t {
  public:
	friend class engine_t;
	using meta_type = core::meta::audio_t;

	audio_t(core::resource::cache_t& cache,
			const core::resource::metadata& metaData,
			core::meta::audio_t* metaFile,
			core::resource::handle<core::audio::engine_t> engine);
	~audio_t();

	bool play();

  private:
	// depending on the internal state of this audio instance we need the engine to exist to safely delete it all.
	core::resource::handle<engine_t> m_Engine;
	std::unique_ptr<float> m_Data;
	std::uint64_t m_Framecount {0};
	std::uint32_t m_SampleRate {0};
	std::uint32_t m_Channels {0};
	mutable void* m_EngineBuffer {nullptr};
	mutable void* m_EngineSound {nullptr};

	format_t m_Format {format_t::unknown};
};
}	 // namespace core::audio
