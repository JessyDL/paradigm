#include "core/ecs/systems/audio.hpp"

#include "core/audio/audio.hpp"
#include "core/ecs/components/audio.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/meta/audio.hpp"
#include "core/resource/resource.hpp"

#include "core/audio/engine.hpp"

#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"
#include "miniaudio.h"

namespace core::ecs::systems {
audio_t::audio_t(psl::ecs::state_t& state, core::resource::handle<core::audio::engine_t> engine) : m_Engine(engine) {
	state.declare<"core::ecs::systems::audio_t::tick_no_loc">(psl::ecs::threading::seq, &audio_t::tick_no_loc, this);
	state.declare<"core::ecs::systems::audio_t::tick_with_loc">(
	  psl::ecs::threading::seq, &audio_t::tick_with_loc, this);
	state.declare<"core::ecs::systems::audio_t::on_mut">(psl::ecs::threading::seq, &audio_t::on_mut, this);
}

void audio_t::tick_no_loc(psl::ecs::info_t& info,
						  psl::ecs::pack_direct_partial_t<const core::ecs::components::audio_resource_t,
														  const core::ecs::components::audio_settings_t,
														  psl::ecs::on_combine<core::ecs::components::audio_resource_t,
																			   core::ecs::components::audio_settings_t>,
														  psl::ecs::except<core::ecs::components::transform>> pack) {
	for(auto [audio, settings] : pack) {
		auto const& instance = audio.audio.value();
		if(!m_Engine->is_initialized(instance) && !m_Engine->init_audio(instance)) {
			core::log->warn("Could not initialize audio object {}", audio.audio.uid());
		}
		if(!m_Engine->is_playing(instance) && !settings.paused) {
			m_Engine->play(instance);
		}
		m_Engine->loop(instance, settings.loop);
		m_Engine->volume(instance, settings.volume);
		m_Engine->pitch(instance, settings.pitch);
	}
}

void audio_t::tick_with_loc(
  psl::ecs::info_t& info,
  psl::ecs::pack_direct_partial_t<psl::ecs::on_combine<const core::ecs::components::audio_resource_t,
													   const core::ecs::components::audio_settings_t,
													   const core::ecs::components::transform>> pack) {}


void audio_t::on_mut(
  psl::ecs::info_t& info,
  psl::ecs::pack_direct_partial_t<const core::ecs::components::audio_resource_t,
								  const core::ecs::components::audio_settings_t,
								  psl::ecs::on_mutate<core::ecs::components::audio_settings_t>> pack) {
	for(auto [audio, settings, mut_info] : pack) {
		if(mut_info.has_mutated<&core::ecs::components::audio_settings_t::volume>()) {
			m_Engine->volume(audio.audio.value(), settings.volume);
		}
		if(mut_info.has_mutated<&core::ecs::components::audio_settings_t::pitch>()) {
			m_Engine->pitch(audio.audio.value(), settings.pitch);
		}
		if(mut_info.has_mutated<&core::ecs::components::audio_settings_t::loop>()) {
			m_Engine->loop(audio.audio.value(), settings.loop);
		}
		if(mut_info.has_mutated<&core::ecs::components::audio_settings_t::paused>()) {
			if(settings.paused) {
				m_Engine->pause(audio.audio.value());
			} else {
				m_Engine->play(audio.audio.value());
			}
		}
	}
}

core::resource::handle<core::audio::engine_t> audio_t::engine() const noexcept {
	return m_Engine;
}
}	 // namespace core::ecs::systems
