#pragma once
#include "ecs/ecs.hpp"
#include "ecs/components/transform.h"
#include "ecs/components/input_tag.h"
#include "systems/input.h"

namespace core::ecs::systems
{
	class fly
	{
		friend class core::systems::input;
	private:
		core::ecs::vector<core::ecs::components::transform, core::ecs::access::READ_WRITE> m_Transforms;

	public:
		fly(core::systems::input& inputSystem);
		~fly();
		void announce(core::ecs::state& state);

		void tick(core::ecs::state& state, const std::vector<core::ecs::entity>& entities, std::chrono::duration<float> dTime);

	private:
		void on_key_pressed(core::systems::input::keycode keyCode);
		void on_key_released(core::systems::input::keycode keyCode);
		void on_mouse_move(core::systems::input::mouse_delta delta);
		void on_scroll(core::systems::input::scroll_delta delta);
		void on_mouse_pressed(core::systems::input::mousecode mCode);
		void on_mouse_released(core::systems::input::mousecode mCode);
		void pitch_to(float degrees);
		void head_to(float degrees);

		core::systems::input& m_InputSystem;

		std::array<bool, 4> m_Moving{false};
		bool m_Up{false};
		psl::vec3 m_MoveVector{0};

		int64_t m_MouseX{1};
		int64_t m_MouseY{1};

		int64_t m_MouseTargetX{1};
		int64_t m_MouseTargetY{1};

		float m_Pitch{ 0.0f };
		float m_Heading{ 0.0f };
		float m_AngleH{ 0.0f };
		float m_AngleV{ 0.0f };

		float m_MoveSpeed{1.0f};
		const float m_MoveStepIncrease{0.035f};
		bool m_AllowRotating{false};
	};
}