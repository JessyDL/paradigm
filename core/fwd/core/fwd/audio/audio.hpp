#pragma once
#include "core/fwd/resource/resource.hpp"

namespace core::meta {
class audio_t;
}

namespace core::audio {
class audio_t;
class engine_t;
}	 // namespace core::audio

namespace core::resource {
template <>
struct resource_traits<core::audio::audio_t> {
	using meta_type = core::meta::audio_t;
};
}	 // namespace core::resource
