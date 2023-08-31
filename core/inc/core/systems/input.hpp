#pragma once
#include "psl/event.hpp"
#include "psl/ustring.hpp"
#if defined(SURFACE_XCB)
	#include <xcb/xcb.h>
#elif defined(SURFACE_WIN32)
	#ifndef _WINDEF_
struct HWND__;	  // Forward or never
typedef HWND__* HWND;
	#endif
#endif

namespace core::os {
class surface;
}

namespace core::systems {
/// \brief handles all inputs and sends out generic events that can be subscribed to.
///
/// the input class is responsible for translating platform specific input events to a more
/// generalised format, and sending out events that have been subscribed to.
/// for example it will translate all keyboard input messages into a generic key message that remains
/// the same for all platforms. You can however reverse-translate back into the platform specific key messages.
/// Every input instance is tied to a core::os::surface, and all its data (such as mouse coordinates) is invariably
/// tied to that regardless if the actual platform only has a singleton input system. \warning all input state
/// persists till the next platform specific message loop. this means that the mouse_delta can be the same for
/// several frames untill the input system gets an update. that's why you should prefer listening to the events
/// rather than iterating the input system itself.
/// \note there are several different events to subscribe to, see the `subscribe` method for further details and documentation.
class input {
	friend class core::os::surface;

  public:
	/// \brief contains the mouse delta coordinates in respect to the last message tick.
	struct mouse_delta {
		int64_t x {0};
		int64_t y {0};
	};

	/// \brief absolute mouse coordinates that also contain the surface width/height.
	struct mouse_coordinate {
		int64_t x {0};
		int64_t y {0};
		uint64_t width {0};
		uint64_t height {0};
	};

	/// \brief codes for the various mouse buttons.
	enum class mousecode {
		LEFT	= 1,
		RIGHT	= 2,
		MIDDLE	= 3,
		EXTRA_1 = 4,
		EXTRA_2 = 5,
		BTN_1	= LEFT,
		BTN_2	= RIGHT,
		BTN_3	= MIDDLE,
		BTN_4	= EXTRA_1,
		BTN_5	= EXTRA_2
	};

	/// \brief delta information for the mouse scroll wheel.
	struct scroll_delta {
		int64_t x {0};
		int64_t y {0};
	};

	/// \brief array that maps keyboard keycodes to string.
	static constexpr psl::string_view keycode_names[] {
	  "",
	  "",
	  "",
	  "",
	  "A",
	  "B",
	  "C",
	  "D",
	  "E",
	  "F",
	  "G",
	  "H",
	  "I",
	  "J",
	  "K",
	  "L",
	  "M",
	  "N",
	  "O",
	  "P",
	  "Q",
	  "R",
	  "S",
	  "T",
	  "U",
	  "V",
	  "W",
	  "X",
	  "Y",
	  "Z",
	  "KB_1",
	  "KB_2",
	  "KB_3",
	  "KB_4",
	  "KB_5",
	  "KB_6",
	  "KB_7",
	  "KB_8",
	  "KB_9",
	  "KB_0",
	  "ENTER",
	  "ESC",
	  "DEL",
	  "TAB",
	  "SPACE",
	  "MINUS",
	  "EQUALS",
	  "LEFT_BRACKET",
	  "RIGHT_BRACKET",
	  "BACK_SLASH",
	  "SEMICOLON",
	  "QUOTE",
	  "GRAVE",
	  "COMMA",
	  "PERIOD",
	  "SLASH",
	  "CAPSLOCK",
	  "F1",
	  "F2",
	  "F3",
	  "F4",
	  "F5",
	  "F6",
	  "F7",
	  "F8",
	  "F9",
	  "F10",
	  "F11",
	  "F12",
	  "PRINT_SCREEN",
	  "SCROLL8LOCK",
	  "PAUSE",
	  "INSERT",
	  "HOME",
	  "PAGE_UP",
	  "DELETE_FORWARD",
	  "END",
	  "PAGE_DOWN",
	  "RIGHT",
	  "LEFT",
	  "DOWN",
	  "UP",
	  "KP_NUM_LOCK",
	  "KP_DIVIDE",
	  "KP_MULTIPLY",
	  "KP_SUBTRACT",
	  "KP_ADD",
	  "KP_ENTER",
	  "KP_1",
	  "KP_2",
	  "KP_3",
	  "KP_4",
	  "KP_5",
	  "KP_6",
	  "KP_7",
	  "KP_8",
	  "KP_9",
	  "KP_0",
	  "KP_POINT",
	  "NON_US_BACKSLASH",
	  "KP_EQUALS",
	  "F13",
	  "F14",
	  "F15",
	  "F16",
	  "F17",
	  "F18",
	  "F19",
	  "F20",
	  "F21",
	  "F22",
	  "F23",
	  "F24",
	  "HELP",
	  "MENU",	 // 118
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 127

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 135

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 143

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 151

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 159

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 167

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 175

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 183

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 191

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 199

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 207

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 215

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 223

	  "LEFT_CONTROL",
	  "LEFT_SHIFT",
	  "LEFT_ALT",
	  "LEFT_GUI",
	  "RIGHT_CONTROL",
	  "RIGHT_SHIFT",
	  "RIGHT_ALT",
	  "RIGHT_GUI",	  // 231

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 239

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",	 // 247

	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "",
	  "UNDEFINED"	 // 255
	};

	/// \brief contains the generic keycodes that platform specific key codes get translated to.
	enum class keycode : uint8_t {
		A				 = 4,
		B				 = 5,
		C				 = 6,
		D				 = 7,
		E				 = 8,
		F				 = 9,
		G				 = 10,
		H				 = 11,
		I				 = 12,
		J				 = 13,
		K				 = 14,
		L				 = 15,
		M				 = 16,
		N				 = 17,
		O				 = 18,
		P				 = 19,
		Q				 = 20,
		R				 = 21,
		S				 = 22,
		T				 = 23,
		U				 = 24,
		V				 = 25,
		W				 = 26,
		X				 = 27,
		Y				 = 28,
		Z				 = 29,
		KB_1			 = 30,
		KB_2			 = 31,
		KB_3			 = 32,
		KB_4			 = 33,
		KB_5			 = 34,
		KB_6			 = 35,
		KB_7			 = 36,
		KB_8			 = 37,
		KB_9			 = 38,
		KB_0			 = 39,
		ENTER			 = 40,
		ESC				 = 41,
		DEL				 = 42,
		TAB				 = 43,
		SPACE			 = 44,
		MINUS			 = 45,
		EQUALS			 = 46,
		LEFT_BRACKET	 = 47,
		RIGHT_BRACKET	 = 48,
		BACK_SLASH		 = 49,
		SEMICOLON		 = 51,
		QUOTE			 = 52,
		GRAVE			 = 53,
		COMMA			 = 54,
		PERIOD			 = 55,
		SLASH			 = 56,
		CAPSLOCK		 = 57,
		F1				 = 58,
		F2				 = 59,
		F3				 = 60,
		F4				 = 61,
		F5				 = 62,
		F6				 = 63,
		F7				 = 64,
		F8				 = 65,
		F9				 = 66,
		F10				 = 67,
		F11				 = 68,
		F12				 = 69,
		PRINT_SCREEN	 = 70,
		SCROLL_LOCK		 = 71,
		PAUSE			 = 72,
		INSERT			 = 73,
		HOME			 = 74,
		PAGE_UP			 = 75,
		DELETE_FORWARD	 = 76,
		END				 = 77,
		PAGE_DOWN		 = 78,
		RIGHT			 = 79,
		LEFT			 = 80,
		DOWN			 = 81,
		UP				 = 82,
		KP_NUM_LOCK		 = 83,
		KP_DIVIDE		 = 84,
		KP_MULTIPLY		 = 85,
		KP_SUBTRACT		 = 86,
		KP_ADD			 = 87,
		KP_ENTER		 = 88,
		KP_1			 = 89,
		KP_2			 = 90,
		KP_3			 = 91,
		KP_4			 = 92,
		KP_5			 = 93,
		KP_6			 = 94,
		KP_7			 = 95,
		KP_8			 = 96,
		KP_9			 = 97,
		KP_0			 = 98,
		KP_POINT		 = 99,
		NON_US_BACKSLASH = 100,
		KP_EQUALS		 = 103,
		F13				 = 104,
		F14				 = 105,
		F15				 = 106,
		F16				 = 107,
		F17				 = 108,
		F18				 = 109,
		F19				 = 110,
		F20				 = 111,
		F21				 = 112,
		F22				 = 113,
		F23				 = 114,
		F24				 = 115,
		HELP			 = 117,
		MENU			 = 118,
		LEFT_CONTROL	 = 224,
		LEFT_SHIFT		 = 225,
		LEFT_ALT		 = 226,
		LEFT_GUI		 = 227,
		RIGHT_CONTROL	 = 228,
		RIGHT_SHIFT		 = 229,
		RIGHT_ALT		 = 230,
		RIGHT_GUI		 = 231,
		UNDEFINED		 = 255
	};

  private:
	template <typename T, typename SFINEA = void>
	struct mf_on_key_pressed : std::false_type {};

	/// \brief SFINAE tag that is used to detect the method signature for the key press event.
	template <typename T>
	struct mf_on_key_pressed<T, std::void_t<decltype(std::declval<T&>().on_key_pressed(std::declval<keycode>()))>>
		: std::true_type {};

	template <typename T, typename SFINEA = void>
	struct mf_on_key_released : std::false_type {};

	/// \brief SFINAE tag that is used to detect the method signature for the key release event.
	template <typename T>
	struct mf_on_key_released<T, std::void_t<decltype(std::declval<T&>().on_key_released(std::declval<keycode>()))>>
		: std::true_type {};

	template <typename T, typename SFINEA = void>
	struct mf_on_key_held : std::false_type {};

	/// \brief SFINAE tag that is used to detect the method signature for the key held event.
	template <typename T>
	struct mf_on_key_held<T, std::void_t<decltype(std::declval<T&>().on_key_held(std::declval<keycode>()))>>
		: std::true_type {};

	template <typename T, typename SFINEA = void>
	struct mf_mouse_delta : std::false_type {};

	/// \brief SFINAE tag that is used to detect the method signature for the mouse delta move event.
	template <typename T>
	struct mf_mouse_delta<T, std::void_t<decltype(std::declval<T&>().on_mouse_move(std::declval<mouse_delta>()))>>
		: std::true_type {};

	template <typename T, typename SFINEA = void>
	struct mf_mouse_coordinate : std::false_type {};

	/// \brief SFINAE tag that is used to detect the method signature for the mouse move (absolute coordinates)
	/// event.
	template <typename T>
	struct mf_mouse_coordinate<
	  T,
	  std::void_t<decltype(std::declval<T&>().on_mouse_move(std::declval<mouse_coordinate>()))>> : std::true_type {};

	template <typename T, typename SFINEA = void>
	struct mf_on_scroll : std::false_type {};

	/// \brief SFINAE tag that is used to detect the method signature for the mouse scroll event.
	template <typename T>
	struct mf_on_scroll<T, std::void_t<decltype(std::declval<T&>().on_scroll(std::declval<scroll_delta>()))>>
		: std::true_type {};

	template <typename T, typename SFINEA = void>
	struct mf_on_mouse_pressed : std::false_type {};

	/// \brief SFINAE tag that is used to detect the method signature for the mouse press event.
	template <typename T>
	struct mf_on_mouse_pressed<T, std::void_t<decltype(std::declval<T&>().on_mouse_pressed(std::declval<mousecode>()))>>
		: std::true_type {};
	template <typename T, typename SFINEA = void>
	struct mf_on_mouse_released : std::false_type {};

	/// \brief SFINAE tag that is used to detect the method signature for the mouse release event.
	template <typename T>
	struct mf_on_mouse_released<T,
								std::void_t<decltype(std::declval<T&>().on_mouse_released(std::declval<mousecode>()))>>
		: std::true_type {};
	template <typename T, typename SFINEA = void>
	struct mf_on_mouse_held : std::false_type {};

	/// \brief SFINAE tag that is used to detect the method signature for the mouse held event.
	template <typename T>
	struct mf_on_mouse_held<T, std::void_t<decltype(std::declval<T&>().on_mouse_held(std::declval<mousecode>()))>>
		: std::true_type {};

  public:
	/// \brief automatically subscribes to all the events the target has method signatures for.
	/// \details The input system checks the target that tries to subscribe itself for several different signatures that satisfy the callbacks.
	/// These are:
	/// - T::on_key_pressed(keycode)			=> when a keycode is pressed (first frame only)
	/// - T::on_key_released(keycode)			=> when a keycode is released (one frame)
	/// - T::on_key_held(keycode)				=> when a keycode is held (first frame after pressed)
	/// - T::on_moude_pressed(mousecode)		=> when a mousecode is pressed (first frame only)
	/// - T::on_mouse_released(mousecode)		=> when a mousecode is released (one frame)
	/// - T::on_mouse_held(mousecode)			=> when a mousecode is held (first frame after pressed)
	/// - T::on_mouse_move(mouse_delta)			=> when the mouse moves, sends the relative coordinates (difference
	/// from last tick)
	/// - T::on_mouse_move(mouse_coordinate)	=> when the mouse moves, sends the absolute coordinates
	/// - T::on_mouse_scroll(scroll_delta)		=> when the mouse moves, sends the delta change of the scroll in
	/// respect to last tick
	template <typename Type>
	void subscribe(Type target) {
		using T = std::remove_pointer_t<std::remove_cvref_t<Type>>;
		T* t {};
		if constexpr(std::is_pointer_v<Type>) {
			t = target;
		} else {
			t = &target;
		}
		if constexpr(mf_on_key_pressed<T>::value)
			m_OnKeyPressed.Subscribe(t, &T::on_key_pressed);
		if constexpr(mf_on_key_released<T>::value)
			m_OnKeyReleased.Subscribe(t, &T::on_key_released);
		if constexpr(mf_on_key_held<T>::value)
			m_OnKeyHeld.Subscribe(t, &T::on_key_held);

		if constexpr(mf_on_mouse_pressed<T>::value)
			m_OnMousePressed.Subscribe(t, &T::on_mouse_pressed);
		if constexpr(mf_on_mouse_released<T>::value)
			m_OnMouseReleased.Subscribe(t, &T::on_mouse_released);
		if constexpr(mf_on_mouse_held<T>::value)
			m_OnMouseHeld.Subscribe(t, &T::on_mouse_held);

		if constexpr(mf_mouse_delta<T>::value)
			m_OnMouseMoveDelta.Subscribe(t, &T::on_mouse_move);
		if constexpr(mf_mouse_coordinate<T>::value)
			m_OnMouseMoveCoordinates.Subscribe(t, &T::on_mouse_move);

		if constexpr(mf_on_scroll<T>::value)
			m_OnScroll.Subscribe(t, &T::on_scroll);

		static_assert(mf_on_key_pressed<T>::value || mf_on_key_released<T>::value || mf_on_key_held<T>::value ||
						mf_on_mouse_pressed<T>::value || mf_on_mouse_released<T>::value || mf_on_mouse_held<T>::value ||
						mf_mouse_delta<T>::value || mf_mouse_coordinate<T>::value || mf_on_scroll<T>::value,
					  "expected at least one event to be subscribable but found none, please check the signatures "
					  "for how the events should look in the documentation.");
	}

	/// \brief automatically unsubscribes to all the events the target has method signatures for.
	template <typename Type>
	void unsubscribe(Type target) {
		using T = std::remove_pointer_t<std::remove_cvref_t<Type>>;
		T* t {};
		if constexpr(std::is_pointer_v<Type>) {
			t = target;
		} else {
			t = &target;
		}

		if constexpr(mf_on_key_pressed<T>::value)
			m_OnKeyPressed.Unsubscribe(t, &T::on_key_pressed);
		if constexpr(mf_on_key_released<T>::value)
			m_OnKeyReleased.Unsubscribe(t, &T::on_key_released);
		if constexpr(mf_on_key_held<T>::value)
			m_OnKeyHeld.Unsubscribe(t, &T::on_key_held);

		if constexpr(mf_on_mouse_pressed<T>::value)
			m_OnMousePressed.Unsubscribe(t, &T::on_mouse_pressed);
		if constexpr(mf_on_mouse_released<T>::value)
			m_OnMouseReleased.Unsubscribe(t, &T::on_mouse_released);
		if constexpr(mf_on_mouse_held<T>::value)
			m_OnMouseHeld.Unsubscribe(t, &T::on_mouse_held);

		if constexpr(mf_mouse_delta<T>::value)
			m_OnMouseMoveDelta.Unsubscribe(t, &T::on_mouse_move);
		if constexpr(mf_mouse_coordinate<T>::value)
			m_OnMouseMoveCoordinates.Unsubscribe(t, &T::on_mouse_move);

		if constexpr(mf_on_scroll<T>::value)
			m_OnScroll.Unsubscribe(t, &T::on_scroll);
	}

	/// \returns the current mouse coordinate information
	const mouse_coordinate& cursor() const noexcept { return m_Cursor; }

  private:
#if defined(SURFACE_WIN32)
	void tick();

	#if defined(PE_PLATFORM_32_BIT)
	static long __stdcall win_event_handler(HWND hWnd, unsigned int uMsg, unsigned int wParam, long lParam);
	#elif defined(PE_PLATFORM_64_BIT)
	static __int64 __stdcall win_event_handler(HWND hWnd, unsigned int uMsg, unsigned __int64 wParam, __int64 lParam);
	#endif

#elif defined(SURFACE_XCB)
	void tick(core::os::surface* const surface, xcb_connection_t* _xcb_connection, xcb_intern_atom_reply_t& delete_wm);
	void handle_event(core::os::surface* const surface,
					  const xcb_generic_event_t* event,
					  xcb_intern_atom_reply_t& delete_wm);
#elif defined(SURFACE_ANDROID)
	void tick();
#endif
	mouse_coordinate m_Cursor;
	common::event<keycode> m_OnKeyPressed;
	common::event<keycode> m_OnKeyReleased;
	common::event<keycode> m_OnKeyHeld;
	common::event<mousecode> m_OnMousePressed;
	common::event<mousecode> m_OnMouseReleased;
	common::event<mousecode> m_OnMouseHeld;
	common::event<mouse_delta> m_OnMouseMoveDelta;
	common::event<mouse_coordinate> m_OnMouseMoveCoordinates;
	common::event<scroll_delta> m_OnScroll;

	std::vector<keycode> m_HeldKeys;
	std::vector<mousecode> m_HeldMouseButtons;
};
}	 // namespace core::systems
