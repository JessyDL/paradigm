#include "psl/terminal_utils.hpp"
#ifdef PE_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
using namespace psl::utility;

void psl::utility::terminal::set_color(color foreground, color background) {
#ifdef PE_PLATFORM_WINDOWS
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (uint8_t)foreground | ((uint8_t)background) * 16u);
#else
	// todo: support unix color codes
	// https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
#endif
}
