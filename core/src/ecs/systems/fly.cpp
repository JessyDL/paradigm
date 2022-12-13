#include "ecs/systems/fly.hpp"
#include "psl/ecs/command_buffer.hpp"
#include <stdint.h>

using namespace core::ecs::systems;
using namespace core::ecs::components;
using namespace psl;
using namespace psl::math;

fly::fly(psl::ecs::state_t& state, core::systems::input& inputSystem) : m_InputSystem(inputSystem)
{
	state.declare<"fly::tick">(&fly::tick, this);
	m_InputSystem.subscribe(this);
};

fly::~fly() { m_InputSystem.unsubscribe(this); }

void fly::tick(
  psl::ecs::info_t& info,
  psl::ecs::pack<core::ecs::components::transform, psl::ecs::filter<core::ecs::components::input_tag>> movables)
{
	bool bHasRotated = m_MouseX != m_MouseTargetX || m_MouseY != m_MouseTargetY;
	if(bHasRotated)
	{
		float mouseSpeed = .12f;
		auto diffX		 = m_MouseX - m_MouseTargetX;
		auto diffY		 = m_MouseY - m_MouseTargetY;
		m_AngleH		 = mouseSpeed * diffX;
		head_to(m_AngleH);

		m_AngleV = mouseSpeed * diffY;
		pitch_to(m_AngleV);
		m_MouseX = m_MouseTargetX;
		m_MouseY = m_MouseTargetY;
	}
	const bool hasMoved = m_Moving[0] || m_Moving[1] || m_Moving[2] || m_Moving[3] || m_Up;
	auto transforms		= movables.get<transform>();
	for(auto& transform : transforms)
	{
		vec3 accDirectionVec {0};
		auto direction = transform.rotation * vec3::forward;
		auto up		   = vec3::up;
		// m_transform.position(m_transform.position() + (m_MoveVector * dTime.count()));
		if(m_Moving[0])
		{
			accDirectionVec += direction;
		}

		if(m_Moving[1])
		{
			accDirectionVec -= direction;
		}

		if(m_Moving[2])
		{
			accDirectionVec -= cross(direction, up);
		}

		if(m_Moving[3])
		{
			accDirectionVec -= cross(up, direction);
		}

		if(m_Up)
		{
			accDirectionVec += up;
		}

		if(hasMoved)
		{
			transform.position = transform.position + normalize(accDirectionVec) *
														((m_Boost) ? m_MoveSpeed * 40 : m_MoveSpeed) *
														info.rTime.count();
			accDirectionVec = psl::vec3::zero;
		}
		if(bHasRotated)
		{
			// determine axis for pitch rotation
			vec3 axis = cross(direction, up);
			// compute quaternion for pitch based on the camera pitch angle
			quat pitch_quat = angle_axis(m_Pitch, axis);
			// determine heading quaternion from the camera up vector and the heading angle
			quat heading_quat = angle_axis(m_Heading, up);
			// add the two quaternions
			// m_Transforms[i].rotation = normalize(pitch_quat * heading_quat);


			// m_Transforms[i].rotation = key_quat * m_Transforms[i].rotation;
			// m_Transforms[i].rotation = normalize(m_Transforms[i].rotation);

			// FPS camera:  RotationX(pitch) * RotationY(yaw)
			psl::quat qPitch   = angle_axis(radians(m_AngleV), axis);
			transform.rotation = normalize(qPitch) * transform.rotation;
			psl::quat qYaw	   = angle_axis(radians(m_AngleH), up);
			transform.rotation = normalize(qYaw) * transform.rotation;
			psl::quat qRoll	   = angle_axis(0.0f, vec3(0, 0, 1));
			transform.rotation = normalize(qRoll) * transform.rotation;

			// For a FPS camera we can omit roll
			// quat m_d_orientation = qPitch * qYaw;
			// quat delta = mix(quat(0, 0, 0, 0), m_d_orientation, dTime.count());
			// m_Transforms[i].rotation = normalize(delta) * m_Transforms[i].rotation;
			// quat orientation = qPitch * qYaw;
			// m_Transforms[i].rotation = normalize(orientation);
		}
	}

	m_AngleH = 0;
	m_AngleV = 0;
	// m_Pitch   = 0;
	// m_Heading = 0;
}

void fly::on_key_pressed(core::systems::input::keycode keyCode)
{
	using keycode_t = core::systems::input::keycode;
	switch(keyCode)
	{
	case keycode_t::Z:
	{
		m_Moving[0] = true;
	}
	break;
	case keycode_t::Q:
	{
		m_Moving[2] = true;
	}
	break;
	case keycode_t::S:
	{
		m_Moving[1] = true;
	}
	break;
	case keycode_t::D:
	{
		m_Moving[3] = true;
	}
	break;
	case keycode_t::SPACE:
	{
		m_Up = true;
	}
	break;
	case keycode_t::LEFT_SHIFT:
	{
		m_Boost = true;
	}
	break;
	default:
		break;
	}
}

void fly::on_key_released(core::systems::input::keycode keyCode)
{
	using keycode_t = core::systems::input::keycode;
	switch(keyCode)
	{
	case keycode_t::Z:
	{
		m_Moving[0] = false;
	}
	break;
	case keycode_t::Q:
	{
		m_Moving[2] = false;
	}
	break;
	case keycode_t::S:
	{
		m_Moving[1] = false;
	}
	break;
	case keycode_t::D:
	{
		m_Moving[3] = false;
	}
	break;
	case keycode_t::SPACE:
	{
		m_Up = false;
	}
	break;
	case keycode_t::LEFT_SHIFT:
	{
		m_Boost = false;
	}
	break;
	default:
		break;
	}
}

void fly::on_mouse_move(core::systems::input::mouse_delta delta)
{
	// if(!m_AllowRotating) return;
	m_MouseTargetX += delta.x;
	m_MouseTargetY += delta.y;
}

void fly::on_scroll(core::systems::input::scroll_delta delta)
{
	m_MoveSpeed += m_MoveStepIncrease * delta.y;
	m_MoveSpeed = std::max(m_MoveSpeed, 0.05f);
}

void fly::on_mouse_pressed(core::systems::input::mousecode mCode)
{
	// if(mCode == core::systems::input::mousecode::RIGHT) m_AllowRotating = true;
}

void fly::on_mouse_released(core::systems::input::mousecode mCode)
{
	// if(mCode == core::systems::input::mousecode::RIGHT) m_AllowRotating = false;
}

void fly::pitch_to(float degrees)
{
	m_Pitch += degrees;

	// Check bounds for the camera pitch
	if(m_Pitch > 360.0f)
	{
		m_Pitch -= 360.0f;
	}
	else if(m_Pitch < -360.0f)
	{
		m_Pitch += 360.0f;
	}
}

void fly::head_to(float degrees)
{
	// This controls how the heading is changed if the camera is pointed straight up or down
	// The heading delta direction changes
	if((m_Pitch > 90 && m_Pitch < 270) || (m_Pitch < -90 && m_Pitch > -270))
	{
		m_Heading += degrees;
	}
	else
	{
		m_Heading -= degrees;
	}
	// Check bounds for the camera heading
	if(m_Heading > 360.0f)
	{
		m_Heading -= 360.0f;
	}
	else if(m_Heading < -360.0f)
	{
		m_Heading += 360.0f;
	}
}
