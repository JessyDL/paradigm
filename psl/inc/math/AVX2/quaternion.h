#pragma once
#if INSTRUCTION_SET == 3
#undef INSTRUCTION_SET
#define INSTRUCTION_SET 0
#include "math/fallback/quaternion.h"
#undef INSTRUCTION_SET
#define INSTRUCTION_SET 3
#endif