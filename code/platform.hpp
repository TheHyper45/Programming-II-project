#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include <cstddef>
#include <cstdint>

namespace core {
	struct Dimensions {
		std::uint32_t width;
		std::uint32_t height;
	};

	struct Point {
		std::uint32_t x;
		std::uint32_t y;
	};

#define CORE_KEYCODES(MACRO)\
	MACRO(Unknown)\
	MACRO(Num_0)\
	MACRO(Num_1)\
	MACRO(Num_2)\
	MACRO(Num_3)\
	MACRO(Num_4)\
	MACRO(Num_5)\
	MACRO(Num_6)\
	MACRO(Num_7)\
	MACRO(Num_8)\
	MACRO(Num_9)\
	MACRO(Numpad_0)\
	MACRO(Numpad_1)\
	MACRO(Numpad_2)\
	MACRO(Numpad_3)\
	MACRO(Numpad_4)\
	MACRO(Numpad_5)\
	MACRO(Numpad_6)\
	MACRO(Numpad_7)\
	MACRO(Numpad_8)\
	MACRO(Numpad_9)\
	MACRO(F1)\
	MACRO(F2)\
	MACRO(F3)\
	MACRO(F4)\
	MACRO(F5)\
	MACRO(F6)\
	MACRO(F7)\
	MACRO(F8)\
	MACRO(F9)\
	MACRO(F10)\
	MACRO(F11)\
	MACRO(F12)\
	MACRO(F13)\
	MACRO(F14)\
	MACRO(F15)\
	MACRO(F16)\
	MACRO(F17)\
	MACRO(F18)\
	MACRO(F19)\
	MACRO(F20)\
	MACRO(F21)\
	MACRO(F22)\
	MACRO(F23)\
	MACRO(F24)\
	MACRO(A)\
	MACRO(B)\
	MACRO(C)\
	MACRO(D)\
	MACRO(E)\
	MACRO(F)\
	MACRO(G)\
	MACRO(H)\
	MACRO(I)\
	MACRO(J)\
	MACRO(K)\
	MACRO(L)\
	MACRO(M)\
	MACRO(N)\
	MACRO(O)\
	MACRO(P)\
	MACRO(Q)\
	MACRO(R)\
	MACRO(S)\
	MACRO(T)\
	MACRO(U)\
	MACRO(V)\
	MACRO(W)\
	MACRO(X)\
	MACRO(Y)\
	MACRO(Z)\
	MACRO(Up)\
	MACRO(Down)\
	MACRO(Left)\
	MACRO(Right)\
	MACRO(Space)\
	MACRO(Delete)\
	MACRO(Insert)\
	MACRO(Return)\
	MACRO(Backspace)\
	MACRO(Left_Shift)\
	MACRO(Right_Shift)\
	MACRO(Left_Alt)\
	MACRO(Right_Alt)\
	MACRO(Left_Ctrl)\
	MACRO(Right_Ctrl)\
	MACRO(Left_Win)\
	MACRO(Right_Win)\
	MACRO(Tab)\
	MACRO(Escape)\
	MACRO(Page_Up)\
	MACRO(Page_Down)\
	MACRO(End)\
	MACRO(Home)\
	MACRO(Select)\
	MACRO(Print)\
	MACRO(Execute)\
	MACRO(Snapshot)\
	MACRO(Help)\
	MACRO(Multiply)\
	MACRO(Add)\
	MACRO(Separator)\
	MACRO(Subtract)\
	MACRO(Decimal)\
	MACRO(Divide)\
	MACRO(Numlock)\
	MACRO(Scrolllock)\
	MACRO(Semicolon)\
	MACRO(Plus)\
	MACRO(Comma)\
	MACRO(Minus)\
	MACRO(Period)\
	MACRO(Slash)\
	MACRO(Grave)\
	MACRO(Left_Bracket)\
	MACRO(Backslash)\
	MACRO(Right_Bracket)\
	MACRO(Quote)\
	MACRO(Oem8)\
	MACRO(Chevrons)\
	MACRO(Clear)\
	MACRO(Capslock)\
	MACRO(Mouse_Left)\
	MACRO(Mouse_Middle)\
	MACRO(Mouse_Right)\
	MACRO(Mouse_X1)\
	MACRO(Mouse_X2)

	enum struct Keycode : std::uint32_t {
#define CORE_KEYCODES_ENUM_VALUE(NAME) NAME,
		CORE_KEYCODES(CORE_KEYCODES_ENUM_VALUE)
#undef CORE_KEYCODES_ENUM_VALUE
		Num_Keycodes
	};
	[[nodiscard]] const char* keycode_to_string(Keycode code);

	class Platform {
	public:
		Platform() noexcept;
		Platform(const Platform&) = delete;
		Platform& operator=(const Platform&) = delete;
		~Platform();
		void create_main_window(const char* title,std::uint32_t width,std::uint32_t height);
		[[nodiscard]] bool window_closed() noexcept;
		[[nodiscard]] bool window_resized() noexcept;
		[[nodiscard]] Dimensions window_client_dimensions() noexcept;
		void process_events();
		void swap_window_buffers();
		void error_message_box(const char* title);
		[[nodiscard]] bool is_key_down(Keycode code) const noexcept;
		[[nodiscard]] bool was_key_pressed(Keycode code) const noexcept;
		[[nodiscard]] Point mouse_position() const noexcept;
	private:
		alignas(std::max_align_t) unsigned char data_buffer[512];
	};
}

#endif
