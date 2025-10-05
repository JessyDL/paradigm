#include "core/audio/engine.hpp"
#include "core/audio/audio.hpp"

#include "core/resource/resource.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace core::audio {
engine_t::engine_t(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile) {
	m_Engine = std::make_unique<ma_engine>();
	ma_result result;
	result = ma_engine_init(nullptr, m_Engine.get());
	if(result != MA_SUCCESS) {
		throw std::runtime_error("Failed to initialize audio engine.");
	}
}

engine_t::~engine_t() {
	ma_engine_uninit(m_Engine.get());
}


bool engine_t::is_initialized(audio_t const& audio) const {
	return audio.m_EngineBuffer && audio.m_EngineSound;
}

bool engine_t::init_audio(audio_t const& audio) const {
	psl_assert(audio.m_EngineSound == nullptr,
			   "Pre-existing engine sound detected in audio, it has already been initialized.");
	ma_audio_buffer_config bufferConfig =
	  ma_audio_buffer_config_init(ma_format_f32, audio.m_Channels, audio.m_Framecount, audio.m_Data.get(), nullptr);

	audio.m_EngineBuffer = new ma_audio_buffer();
	auto* buffer		 = (ma_audio_buffer*)audio.m_EngineBuffer;

	auto result = ma_audio_buffer_init(&bufferConfig, buffer);
	if(result != MA_SUCCESS) {
		delete((ma_audio_buffer*)audio.m_EngineBuffer);
		audio.m_EngineBuffer = nullptr;
		core::log->error("Failed to initialize audio buffer.");
		return false;
	}

	audio.m_EngineSound = new ma_sound();
	result				= ma_sound_init_from_data_source(
	   m_Engine.get(), buffer, MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, (ma_sound*)audio.m_EngineSound);
	if(result != MA_SUCCESS) {
		delete((ma_audio_buffer*)audio.m_EngineBuffer);
		audio.m_EngineBuffer = nullptr;
		delete((ma_sound*)audio.m_EngineSound);
		audio.m_EngineSound = nullptr;
		core::log->error("Failed to initialize sound.");
		return false;
	}
	return true;
}
void engine_t::deinit_audio(audio_t const& audio) const {
	if(audio.m_EngineSound) {
		ma_sound_uninit((ma_sound*)audio.m_EngineSound);
		delete((ma_sound*)audio.m_EngineSound);
		audio.m_EngineSound = nullptr;
		delete((ma_audio_buffer*)audio.m_EngineBuffer);
		audio.m_EngineBuffer = nullptr;
	}
}

bool engine_t::play(audio_t const& audio) const {
	if(!audio.m_EngineSound) {
		return false;
	}
	if(ma_sound_start((ma_sound*)audio.m_EngineSound) != MA_SUCCESS) {
		deinit_audio(audio);
		return false;
	}
	return true;
}


bool engine_t::is_playing(audio_t const& audio) const {
	if(!audio.m_EngineSound) {
		return false;
	}

	return ma_sound_is_playing((ma_sound*)audio.m_EngineSound);
}
void engine_t::volume(audio_t const& audio, float value) const {
	if(!audio.m_EngineSound) {
		return;
	}

	ma_sound_set_volume((ma_sound*)audio.m_EngineSound, value);
}
float engine_t::volume(audio_t const& audio) const {
	if(!audio.m_EngineSound) {
		return false;
	}

	return ma_sound_get_volume((ma_sound*)audio.m_EngineSound);
}

void engine_t::pause(audio_t const& audio) const {
	if(!audio.m_EngineSound) {
		return;
	}
	ma_sound_stop((ma_sound*)audio.m_EngineSound);
}

void engine_t::reset(audio_t const& audio) const {
	if(!audio.m_EngineSound) {
		return;
	}

	ma_sound_seek_to_pcm_frame((ma_sound*)audio.m_EngineSound, 0);
}

void engine_t::loop(audio_t const& audio, bool value) const {
	if(!audio.m_EngineSound) {
		return;
	}
	if(value) {
		ma_sound_set_looping((ma_sound*)audio.m_EngineSound, MA_TRUE);
	} else {
		ma_sound_set_looping((ma_sound*)audio.m_EngineSound, MA_FALSE);
	}
}

bool engine_t::seek(audio_t const& audio, float seconds) const {
	if(!audio.m_EngineSound) {
		return false;
	}

	ma_uint64 frame = (ma_uint64)(seconds * audio.m_SampleRate);
	return ma_sound_seek_to_pcm_frame((ma_sound*)audio.m_EngineSound, frame) == MA_SUCCESS;
}

void engine_t::pitch(audio_t const& audio, float value) const {
	if(!audio.m_EngineSound) {
		return;
	}
	ma_sound_set_pitch((ma_sound*)audio.m_EngineSound, value);
}

float engine_t::pitch(audio_t const& audio) const {
	return ma_sound_get_pitch((ma_sound*)audio.m_EngineSound);
}
}	 // namespace core::audio
