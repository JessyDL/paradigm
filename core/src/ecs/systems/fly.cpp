#pragma once
#include "stdafx.h"
#include "ecs/systems/fly.h"
using namespace core::ecs::systems;
using namespace core::ecs::components;
using namespace psl;
using namespace psl::math;

fly::fly(core::systems::input& inputSystem) : m_InputSystem(inputSystem) 
{ 
	m_InputSystem.subscribe(this); 
};

fly::~fly() { m_InputSystem.unsubscribe(this); }

void fly::announce(core::ecs::state& state)
{
	state.register_dependency(m_Transforms);
	state.register_dependency<core::ecs::components::input_tag>();
}

void fly::tick(core::ecs::state& state, const std::vector<core::ecs::entity>& entities,
			   std::chrono::duration<float> dTime)
{
	if (m_MouseX != m_MouseTargetX || m_MouseY != m_MouseTargetY)
	{
		float mouseSpeed = 0.004f;
		auto diffX = m_MouseX - m_MouseTargetX;
		auto diffY = m_MouseY - m_MouseTargetY;
		m_AngleH = mouseSpeed * diffX;
		head_to(-m_AngleH);

		m_AngleV = mouseSpeed * diffY;
		pitch_to(m_AngleV);
		m_MouseX = m_MouseTargetX;
		m_MouseY = m_MouseTargetY;

	}

	for(size_t i = 0; i < entities.size(); ++i)
	{
		// m_Transform.position(m_Transform.position() + (m_MoveVector * dTime.count()));
		if(m_Moving[0])
		{
			vec3 forward = m_Transforms[i].direction;
			// forward.y = 0;
			forward					 = normalize(forward);
			m_Transforms[i].position = m_Transforms[i].position + forward * m_MoveSpeed * dTime.count();
		}

		if(m_Moving[1])
		{
			vec3 forward = m_Transforms[i].direction;
			// forward.y = 0;
			forward					 = normalize(forward);
			m_Transforms[i].position = m_Transforms[i].position - forward * m_MoveSpeed * dTime.count();
		}

		if(m_Moving[2])
		{
			vec3 Left = cross(m_Transforms[i].direction, m_Transforms[i].up);
			Left	  = normalize(Left);
			Left *= m_MoveSpeed * dTime.count();
			m_Transforms[i].position = m_Transforms[i].position - Left;
		}

		if(m_Moving[3])
		{
			vec3 Right = cross(m_Transforms[i].up, m_Transforms[i].direction);
			Right	  = normalize(Right);
			Right *= m_MoveSpeed * dTime.count();
			m_Transforms[i].position = m_Transforms[i].position - Right;
		}

		if(m_Up)
		{
			m_Transforms[i].position = m_Transforms[i].position + vec3::up * dTime.count() * 1.0f * m_MoveSpeed;
		}

		if(m_AngleH != 0 || m_AngleV != 0)
		{
			// determine axis for pitch rotation
			vec3 axis = cross(m_Transforms[i].direction, m_Transforms[i].up);
			//core::log->info("ecs axis: {0} {1} {2}", axis[0], axis[1], axis[2]);
			// compute quaternion for pitch based on the camera pitch angle
			quat pitch_quat = angle_axis(m_Pitch, axis);

			//core::log->info("ecs pitch_quat: {0} {1} {2} {3}", pitch_quat[0], pitch_quat[1], pitch_quat[2], pitch_quat[3]);
			// determine heading quaternion from the camera up vector and the heading angle
			quat heading_quat = angle_axis(m_Heading, m_Transforms[i].up);
			// add the two quaternions
			quat temp				 = normalize(cross(pitch_quat, heading_quat));
			m_Transforms[i].direction = normalize(rotate(temp, m_Transforms[i].direction));
			m_Transforms[i].rotation = temp;			
		}
	}

	m_AngleH  = 0;
	m_AngleV  = 0;
	m_Pitch   = 0;
	m_Heading = 0;
}

void fly::on_key_pressed(core::systems::input::keycode keyCode)
{
	using keycode_t = core::systems::input::keycode;
	switch(keyCode)
	{
	case keycode_t::Z: { m_Moving[0] = true;
	}
	break;
	case keycode_t::Q: { m_Moving[2] = true;
	}
	break;
	case keycode_t::S: { m_Moving[1] = true;
	}
	break;
	case keycode_t::D: { m_Moving[3] = true;
	}
	break;
	case keycode_t::SPACE: { m_Up = true;
	}
	break;
	default: break;
	}
}

void fly::on_key_released(core::systems::input::keycode keyCode)
{
	using keycode_t = core::systems::input::keycode;
	switch(keyCode)
	{
	case keycode_t::Z: { m_Moving[0] = false;
	}
	break;
	case keycode_t::Q: { m_Moving[2] = false;
	}
	break;
	case keycode_t::S: { m_Moving[1] = false;
	}
	break;
	case keycode_t::D: { m_Moving[3] = false;
	}
	break;
	case keycode_t::SPACE: { m_Up = false;
	}
	break;
	default: break;
	}
}

void fly::on_mouse_move(core::systems::input::mouse_delta delta)
{
	if(!m_AllowRotating) return;
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
	if(mCode == core::systems::input::mousecode::RIGHT) m_AllowRotating = true;
}

void fly::on_mouse_released(core::systems::input::mousecode mCode)
{
	if(mCode == core::systems::input::mousecode::RIGHT) m_AllowRotating = false;
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