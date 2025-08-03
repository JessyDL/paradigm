#include "core/audio/audio.hpp"
#include "core/audio/engine.hpp"
#include "core/meta/audio.hpp"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace core::audio {
audio_t::audio_t(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 core::meta::audio_t* metaFile,
				 core::resource::handle<engine_t> engine)
	: m_Engine(engine), m_Data(nullptr) {
	auto data = cache.library().load(metaFile->ID());
	m_Format  = metaFile->format();

	// here to trick msvc into not removing this static member var during its optimization
	static auto val = []() { return core::meta::audio_t::polymorphic_identity; }();

	float* res = nullptr;

	switch(m_Format) {
	case format_t::mp3: {
		drmp3_config config;
		drmp3_uint64 frameCount;
		res = drmp3_open_memory_and_read_pcm_frames_f32(
		  (float*)data.value().data(), data.value().size(), &config, &frameCount, nullptr);

		m_Channels	 = config.channels;
		m_SampleRate = config.sampleRate;
		m_Framecount = psl::narrow_cast<std::uint64_t>(frameCount);
	} break;
	case format_t::flac: {
		drflac_uint64 frameCount;
		res = drflac_open_memory_and_read_pcm_frames_f32(
		  (float*)data.value().data(), data.value().size(), &m_Channels, &m_SampleRate, &frameCount, nullptr);
		m_Framecount = psl::narrow_cast<std::uint64_t>(frameCount);
	} break;
	case format_t::wav: {
		drwav_uint64 frameCount;
		res = drwav_open_memory_and_read_pcm_frames_f32(
		  (float*)data.value().data(), data.value().size(), &m_Channels, &m_SampleRate, &frameCount, nullptr);
		m_Framecount = psl::narrow_cast<std::uint64_t>(frameCount);
	} break;
	default:
		core::log->critical("Could not identify the audio format type '{}' for file {}",
							psl::to_underlying(m_Format),
							metaData.resource_uid.to_string());
		std::abort();
	}

	m_Data = std::unique_ptr<float>(res);
	if(!m_Data) {
		m_Channels	 = 0;
		m_SampleRate = 0;
		m_Framecount = 0;
		throw std::runtime_error("Failed to load audio data from memory.");
	}
}

audio_t::~audio_t() {
	m_Engine->deinit_audio(*this);
	if(m_Data) {
		switch(m_Format) {
		case format_t::mp3:
			drmp3_free(m_Data.get(), nullptr);
			break;
		case format_t::flac:
			drflac_free(m_Data.get(), nullptr);
			break;
		case format_t::wav:
			drwav_free(m_Data.get(), nullptr);
			break;
		default:
			core::log->critical("Unhandled free-ing of format, you need to implement this.");
			std::abort();
		}
		m_Data.release();
	}
}

bool audio_t::play() {
	if(m_Engine->init_audio(*this)) {
		return m_Engine->play(*this);
	}
	return false;
}
}	 // namespace core::audio
