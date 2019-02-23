#pragma once


namespace psl::ecs
{
	class state;

	class command_buffer
	{
	public:
		command_buffer(const state& state);
	private:

		state const * m_State;
	};
}