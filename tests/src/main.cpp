#ifdef PE_PLATFORM_WINDOWS
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
	#include <stdlib.h>
#endif

#include <litmus/litmus.hpp>
#ifdef PE_PLATFORM_WEB
	#include <vector>
#endif

LITMUS_EXTERN();

int main(int argc, char* argv[]) {
#ifdef PE_PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
#ifdef PE_PLATFORM_WEB
	std::vector<char*> args {};
	using namespace std::string_view_literals;
	std::vector<std::string> additional_args_storage {{"--no-source", "--single-threaded", "--formatter", "compact"}};
	for(size_t i = 0; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}
	for(auto& arg : additional_args_storage) {
		args.emplace_back(arg.data());
	}

	return litmus::run(args.size(), args.data());
#else
	return litmus::run(argc, argv);
#endif
}
