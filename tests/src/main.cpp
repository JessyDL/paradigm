#ifdef PE_PLATFORM_WINDOWS
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
	#include <stdlib.h>
#endif

#include <litmus/litmus.hpp>

LITMUS_EXTERN();

int main(int argc, char* argv[]) {
#ifdef PE_PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	return litmus::run(argc, argv);
}
