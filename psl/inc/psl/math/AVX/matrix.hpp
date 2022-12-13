#pragma once
#if INSTRUCTION_SET == 2
	#undef INSTRUCTION_SET
	#define INSTRUCTION_SET 1
	#include "math/SSE/matrix.hpp"
	#undef INSTRUCTION_SET
	#define INSTRUCTION_SET 2
#endif
