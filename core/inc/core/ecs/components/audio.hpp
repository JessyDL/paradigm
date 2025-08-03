#pragma once
#include "core/resource/handle.hpp"
#include "psl/ecs/component_traits.hpp"

// We separate the audio_resource_t and audio_settings_t components as the resource is by its nature a complex type,
// and mutability traits/filters can only be applied to types which satisfy the IsComponentTrivialType concept.

namespace core::audio {
class audio_t;
}

namespace core::ecs::components {

struct audio_resource_t {
	core::resource::handle<core::audio::audio_t> audio {};
};
struct audio_settings_t {
	static audio_settings_t prototype() {
		return audio_settings_t {.volume = 1.0f, .pitch = 1.0f, .loop = false, .paused = true};
	}
	float volume;
	float pitch;
	bool loop;
	bool paused;
};
}	 // namespace core::ecs::components

template <>
struct psl::ecs::component_trait_mutability_t<core::ecs::components::audio_settings_t> {
	static constexpr component_mutability_behaviour_t mutability {component_mutability_behaviour_t::restricted};
};
