#pragma once
#include "core/fwd/resource/resource.hpp"
#include "psl/ecs/state.hpp"

namespace core::audio {
class engine_t;
}

namespace core::ecs::components {
struct transform;
struct audio_resource_t;
struct audio_settings_t;
}	 // namespace core::ecs::components

namespace core::ecs::systems {
class audio_t {
  public:
	audio_t(psl::ecs::state_t& state, core::resource::handle<core::audio::engine_t> engine);
	~audio_t() = default;

	audio_t(audio_t const&)			   = delete;
	audio_t(audio_t&&)				   = delete;
	audio_t& operator=(audio_t const&) = delete;
	audio_t& operator=(audio_t&&)	   = delete;

	core::resource::handle<core::audio::engine_t> engine() const noexcept;

  private:
	// plays audio globally if no transform is present
	void tick_no_loc(psl::ecs::info_t& info,
					 psl::ecs::pack_direct_partial_t<const core::ecs::components::audio_resource_t,
													 const core::ecs::components::audio_settings_t,
													 psl::ecs::on_combine<core::ecs::components::audio_resource_t,
																		  core::ecs::components::audio_settings_t>,
													 psl::ecs::except<core::ecs::components::transform>> pack);

	void on_mut(psl::ecs::info_t& info,
				psl::ecs::pack_direct_partial_t<const core::ecs::components::audio_resource_t,
												const core::ecs::components::audio_settings_t,
												psl::ecs::on_mutate<core::ecs::components::audio_settings_t>> pack);

	// plays audio globally _only_ if the audio says it should be.
	void
	tick_with_loc(psl::ecs::info_t& info,
				  psl::ecs::pack_direct_partial_t<psl::ecs::on_combine<const core::ecs::components::audio_resource_t,
																	   const core::ecs::components::audio_settings_t,
																	   const core::ecs::components::transform>> pack);

	core::resource::handle<core::audio::engine_t> m_Engine;
};
}	 // namespace core::ecs::systems
