#pragma once
#include "core/ecs/components/input_tag.hpp"
#include "core/ecs/components/transform.hpp"
#include "psl/ecs/state.hpp"
#include "core/systems/input.hpp"

namespace core::ecs::systems {
class fly {
	friend class core::systems::input;

  public:
	fly(psl::ecs::state_t& state, core::systems::input& inputSystem);
	~fly();


  private:
	void tick(psl::ecs::info_t& info,
			  psl::ecs::pack_direct_full_t<core::ecs::components::transform,
										   psl::ecs::filter<core::ecs::components::input_tag>> movables);

	void on_key_pressed(core::systems::input::keycode keyCode);
	void on_key_released(core::systems::input::keycode keyCode);
	void on_mouse_move(core::systems::input::mouse_delta delta);
	void on_scroll(core::systems::input::scroll_delta delta);
	void on_mouse_pressed(core::systems::input::mousecode mCode);
	void on_mouse_released(core::systems::input::mousecode mCode);
	void pitch_to(float degrees);
	void head_to(float degrees);

	core::systems::input& m_InputSystem;

	std::array<bool, 4> m_Moving {false};
	bool m_Up {false};
	psl::vec3 m_MoveVector {0};

	int64_t m_MouseX {1};
	int64_t m_MouseY {1};

	int64_t m_MouseTargetX {1};
	int64_t m_MouseTargetY {1};

	float m_Pitch {0.0f};
	float m_Heading {0.0f};
	float m_AngleH {0.0f};
	float m_AngleV {0.0f};

	float m_MoveSpeed {1.0f};
	const float m_MoveStepIncrease {0.035f};
	bool m_AllowRotating {false};
	bool m_Boost {false};
};
}	 // namespace core::ecs::systems
