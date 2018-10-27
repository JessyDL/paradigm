#include "stdafx_psl.h"
#include "terminal_utils.h"

using namespace utility;

void utility::terminal::set_color(color foreground, color background)
{
#ifdef PLATFORM_WINDOWS
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (uint8_t)foreground | ((uint8_t)background)*16u);
#else
	// todo: support unix color codes https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
#endif
}
