#include "ecs/command_buffer.h"

using namespace psl::ecs;

command_buffer::command_buffer(const state& state) : m_State(&state) {};