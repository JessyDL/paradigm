// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "stdafx.h"

#ifdef DBG_NEW
#undef new
#endif
#include <catch2/catch.hpp>
#ifdef DBG_NEW
#define new DBG_NEW
#endif
// TODO: reference additional headers your program requires here
