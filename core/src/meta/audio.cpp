#include "core/meta/audio.hpp"
#include "core/resource/resource.hpp"

using namespace core::meta;
using namespace psl::serialization;

volatile const uint64_t audio_t::polymorphic_identity {register_polymorphic<audio_t>()};
