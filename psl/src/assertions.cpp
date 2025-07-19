#include "psl/assertions.hpp"


#if !defined(PE_PLATFORM_ANDROID)
	#include <iostream>

void psl::details::print_to_cout(std::string_view message) {
	std::cout << message << std::endl;
}
#endif
