#pragma once
#if INSTRUCTION_SET == 3
	#undef INSTRUCTION_SET
	#define INSTRUCTION_SET 2
	#include "math/AVX/matrix.hpp"
	#undef INSTRUCTION_SET
	#define INSTRUCTION_SET 3
#endif
