#pragma once
#include "core/fwd/resource/resource.hpp"

struct ma_engine;
namespace psl::meta {
class file;
}

namespace core::resource {
struct metadata;
}

namespace core::audio {
class audio_t;
class engine_t {
  public:
	engine_t(core::resource::cache_t& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile);
	~engine_t();

	bool is_initialized(audio_t const& audio) const;
	bool init_audio(audio_t const& audio) const;
	void deinit_audio(audio_t const& audio) const;
	bool play(audio_t const& audio) const;
	bool is_playing(audio_t const& audio) const;
	void volume(audio_t const& audio, float value) const;
	float volume(audio_t const& audio) const;
	void pause(audio_t const& audio) const;
	void reset(audio_t const& audio) const;
	void loop(audio_t const& audio, bool value) const;
	bool seek(audio_t const& audio, float seconds) const;
	void pitch(audio_t const& audio, float value) const;
	float pitch(audio_t const& audio) const;

  private:
	std::unique_ptr<ma_engine> m_Engine;
};
}	 // namespace core::audio
