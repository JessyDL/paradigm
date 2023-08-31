#pragma once
#include <stdint.h>

namespace psl::utility {
class terminal {
  public:
	terminal()	= delete;
	~terminal() = delete;

	enum class color : uint8_t {
		BLACK		 = 0x0,
		BLUE		 = 0x1,
		GREEN		 = 0x2,
		CYAN		 = 0x3,
		RED			 = 0x4,
		MAGENTA		 = 0x5,
		BROWN		 = 0x6,
		LIGHTGRAY	 = 0x7,
		DARKGRAY	 = 0x8,
		LIGHTBLUE	 = 0x9,
		LIGHTGREEN	 = 10,
		LIGHTCYAN	 = 11,
		LIGHTRED	 = 12,
		LIGHTMAGENTA = 13,
		YELLOW		 = 14,
		WHITE		 = 15,
	};

	static void set_color(color foreground, color background = color::BLACK);
};
}	 // namespace psl::utility
