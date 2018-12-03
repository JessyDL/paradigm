#pragma once
#if INSTRUCTION_SET == 2
#undef INSTRUCTION_SET
#define INSTRUCTION_SET 0
#include "math/fallback/vec.h"
#undef INSTRUCTION_SET
#define INSTRUCTION_SET 2
#endif