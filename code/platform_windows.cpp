#include <cstring>
#include <iostream>
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <Windows.h>
#include <Windowsx.h>
#undef near
#undef far
#include "defer.hpp"
#include "opengl.hpp"
#include "wglext.h"
#include "platform.hpp"
#include "renderer.hpp"
#include "exceptions.hpp"

namespace core {
	static constexpr const char* Window_Class_Name = "CustomWindowClass";
	static constexpr DWORD Window_Flags = WS_BORDER | WS_SYSMENU | WS_CAPTION | WS_SIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE;
	static constexpr int Array_End_Marker = 0;

	const char* keycode_to_string(Keycode code) {
		switch(code) {
#define CORE_KEYCODES_ENUM_CASE(NAME) case Keycode::NAME: return #NAME;
			CORE_KEYCODES(CORE_KEYCODES_ENUM_CASE)
#undef CORE_KEYCODES_ENUM_CASE
		}
		return "[Invalid]";
	}

	struct Platform_Windows_Data {
		HWND window;
		HDC dc;
		HGLRC ctx;
		bool window_closed;
		bool window_resized;
		Dimensions window_client_dims;
		bool key_down_statuses[std::size_t(Keycode::Num_Keycodes)];
		bool was_key_pressed_statuses[std::size_t(Keycode::Num_Keycodes)];
		Point mouse_position;
	};

	[[nodiscard]] static Keycode vk_code_to_keycode(WORD vk_code) {
		if(vk_code >= '0' && vk_code <= '9')
			return Keycode(std::uint32_t(Keycode::Num_0) + (vk_code - '0'));
		if(vk_code >= 'A' && vk_code <= 'Z')
			return Keycode(std::uint32_t(Keycode::A) + (vk_code - 'A'));
		if(vk_code >= VK_F1 && vk_code <= VK_F24)
			return Keycode(std::uint32_t(Keycode::F1) + (vk_code - VK_F1));
		if(vk_code >= VK_NUMPAD0 && vk_code <= VK_NUMPAD9)
			return Keycode(std::uint32_t(Keycode::Numpad_0) + (vk_code - VK_NUMPAD0));

		//@TODO: Figure out what these are.
		if(vk_code >= 0x92 && vk_code <= 0x96)
			return Keycode::Num_Keycodes;
		if(vk_code == 0xE1)
			return Keycode::Num_Keycodes;
		if(vk_code >= 0xE3 && vk_code <= 0xE4)
			return Keycode::Num_Keycodes;
		if(vk_code == 0xE6)
			return Keycode::Num_Keycodes;
		if(vk_code >= 0xE9 && vk_code <= 0xF5)
		   return Keycode::Num_Keycodes;

		switch(vk_code) {
			case VK_UP: return Keycode::Up;
			case VK_DOWN: return Keycode::Down;
			case VK_LEFT: return Keycode::Left;
			case VK_RIGHT: return Keycode::Right;
			case VK_SPACE: return Keycode::Space;
			case VK_DELETE: return Keycode::Delete;
			case VK_INSERT: return Keycode::Insert;
			case VK_RETURN: return Keycode::Return;
			case VK_BACK: return Keycode::Backspace;
			case VK_LSHIFT: return Keycode::Left_Shift;
			case VK_RSHIFT: return Keycode::Right_Shift;
			case VK_LMENU: return Keycode::Left_Alt;
			case VK_RMENU: return Keycode::Right_Alt;
			case VK_LCONTROL: return Keycode::Left_Ctrl;
			case VK_RCONTROL: return Keycode::Right_Ctrl;
			case VK_LWIN: return Keycode::Left_Win;
			case VK_RWIN: return Keycode::Right_Win;
			case VK_TAB: return Keycode::Tab;
			case VK_ESCAPE: return Keycode::Escape;
			case VK_PRIOR: return Keycode::Page_Up;
			case VK_NEXT: return Keycode::Page_Down;
			case VK_END: return Keycode::End;
			case VK_HOME: return Keycode::Home;
			case VK_SELECT: return Keycode::Select;
			case VK_PRINT: return Keycode::Print;
			case VK_EXECUTE: return Keycode::Execute;
			case VK_SNAPSHOT: return Keycode::Snapshot;
			case VK_HELP: return Keycode::Help;
			case VK_MULTIPLY: return Keycode::Multiply;
			case VK_ADD: return Keycode::Add;
			case VK_SEPARATOR: return Keycode::Separator;
			case VK_SUBTRACT: return Keycode::Subtract;
			case VK_DECIMAL: return Keycode::Decimal;
			case VK_DIVIDE: return Keycode::Divide;
			case VK_NUMLOCK: return Keycode::Numlock;
			case VK_SCROLL: return Keycode::Scrolllock;
			case VK_OEM_1: return Keycode::Semicolon;
			case VK_OEM_PLUS: return Keycode::Plus;
			case VK_OEM_COMMA: return Keycode::Comma;
			case VK_OEM_MINUS: return Keycode::Minus;
			case VK_OEM_PERIOD: return Keycode::Period;
			case VK_OEM_2: return Keycode::Slash;
			case VK_OEM_3: return Keycode::Grave;
			case VK_OEM_4: return Keycode::Left_Bracket;
			case VK_OEM_5: return Keycode::Backslash;
			case VK_OEM_6: return Keycode::Right_Bracket;
			case VK_OEM_7: return Keycode::Quote;
			case VK_OEM_8: return Keycode::Oem8; //@TODO: Figure out what this is.
			case VK_OEM_102: return Keycode::Chevrons;
			case VK_OEM_CLEAR: return Keycode::Clear;
			case VK_CAPITAL: return Keycode::Capslock;
		}
		return Keycode::Unknown;
	}

	static LRESULT CALLBACK window_proc(HWND window,UINT msg,WPARAM wparam,LPARAM lparam) {
		Platform_Windows_Data* data = reinterpret_cast<Platform_Windows_Data*>(GetWindowLongPtrA(window,0));
		switch(msg) {
			case WM_CLOSE: {
				data->window_closed = true;
				return 0;
			}
			case WM_SIZE: {
				if(data) {
					RECT rect = {};
					GetClientRect(window,&rect);
					data->window_client_dims = {std::uint32_t(rect.right),std::uint32_t(rect.bottom)};
					data->window_resized = true;
				}
				return 0;
			}
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYUP: {
				WORD vk_code = LOWORD(wparam);
				WORD key_flags = HIWORD(lparam);
				BOOL was_key_down = (key_flags & KF_REPEAT) == KF_REPEAT;
				BOOL is_key_released = (key_flags & KF_UP) == KF_UP;

				if(vk_code == VK_SHIFT || vk_code == VK_CONTROL || vk_code == VK_MENU) {
					WORD scan_code = LOBYTE(key_flags);
					if((key_flags & KF_EXTENDED) == KF_EXTENDED) scan_code = MAKEWORD(scan_code,0xE0);
					vk_code = LOWORD(MapVirtualKeyA(scan_code,MAPVK_VSC_TO_VK_EX));
				}

				auto keycode = core::vk_code_to_keycode(vk_code);
				if(was_key_down == 0 && is_key_released == 0) {
					data->was_key_pressed_statuses[std::size_t(keycode)] = true;
					data->key_down_statuses[std::size_t(keycode)] = true;
					//std::cout << "Keycode::" << core::keycode_to_string(keycode) << std::endl;
				}
				else if(was_key_down == 1 && is_key_released == 0) {
					data->key_down_statuses[std::size_t(keycode)] = true;
				}
				else if(was_key_down == 1 && is_key_released == 1) {
					data->key_down_statuses[std::size_t(keycode)] = false;
				}

				if(msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) return DefWindowProcA(window,msg,wparam,lparam);
				return 0;
			}
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_XBUTTONDOWN: {
				if((wparam & MK_LBUTTON) == MK_LBUTTON) {
					data->was_key_pressed_statuses[std::size_t(Keycode::Mouse_Left)] = true;
					data->key_down_statuses[std::size_t(Keycode::Mouse_Left)] = true;
				}
				if((wparam & MK_MBUTTON) == MK_MBUTTON) {
					data->was_key_pressed_statuses[std::size_t(Keycode::Mouse_Middle)] = true;
					data->key_down_statuses[std::size_t(Keycode::Mouse_Middle)] = true;
				}
				if((wparam & MK_RBUTTON) == MK_RBUTTON) {
					data->was_key_pressed_statuses[std::size_t(Keycode::Mouse_Right)] = true;
					data->key_down_statuses[std::size_t(Keycode::Mouse_Right)] = true;
				}
				if((wparam & MK_XBUTTON1) == MK_XBUTTON1) {
					data->was_key_pressed_statuses[std::size_t(Keycode::Mouse_X1)] = true;
					data->key_down_statuses[std::size_t(Keycode::Mouse_X1)] = true;
				}
				if((wparam & MK_XBUTTON2) == MK_XBUTTON2) {
					data->was_key_pressed_statuses[std::size_t(Keycode::Mouse_X2)] = true;
					data->key_down_statuses[std::size_t(Keycode::Mouse_X2)] = true;
				}
				return msg == WM_XBUTTONDOWN;
			}
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			case WM_XBUTTONUP: {
				if((wparam & MK_LBUTTON) != MK_LBUTTON)
					data->key_down_statuses[std::size_t(Keycode::Mouse_Left)] = false;
				if((wparam & MK_MBUTTON) != MK_MBUTTON)
					data->key_down_statuses[std::size_t(Keycode::Mouse_Middle)] = false;
				if((wparam & MK_RBUTTON) != MK_RBUTTON)
					data->key_down_statuses[std::size_t(Keycode::Mouse_Right)] = false;
				if((wparam & MK_XBUTTON1) != MK_XBUTTON1)
					data->key_down_statuses[std::size_t(Keycode::Mouse_X1)] = false;
				if((wparam & MK_XBUTTON2) != MK_XBUTTON2)
					data->key_down_statuses[std::size_t(Keycode::Mouse_X2)] = false;
				return msg == WM_XBUTTONUP;
			}
			case WM_MOUSEMOVE: {
				data->mouse_position = {std::uint32_t(GET_X_LPARAM(lparam)),std::uint32_t(GET_Y_LPARAM(lparam))};
				return 0;
			}
			case WM_CHAR: {
				//std::cout << wparam << std::endl;
				return 0;
			}
		}
		return DefWindowProcA(window,msg,wparam,lparam);
	}

	Platform::Platform() noexcept : data_buffer() {
		static_assert(sizeof(Platform_Windows_Data) <= sizeof(data_buffer));
		SetConsoleOutputCP(CP_UTF8);
		new(data_buffer) Platform_Windows_Data();
	}

	Platform::~Platform() {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		wglMakeCurrent(data.dc,nullptr);
		wglDeleteContext(data.ctx);
		ReleaseDC(data.window,data.dc);
		DestroyWindow(data.window);
	}

	[[nodiscard]] static HGLRC create_opengl_context(Platform_Windows_Data& data) {
		//OpenGL initialization on Windows is really weird.
		//If you want to use newer versions of OpenGL you have to use 'wglChoosePixelFormatARB' and 'wglCreateContextAttribsARB' to create a context.
		//But to get function pointers to these functions you have to use 'wglGetProcAddress' which only works if you already have a context.
		//So we need to create an invisible window, initialize OpenGL the old way, get function pointers, create the proper window and destroy the temporary window.

		HWND temp_window = CreateWindowExA(0,Window_Class_Name,"Temporary",WS_POPUP,0,0,1,1,HWND_DESKTOP,nullptr,GetModuleHandleA(nullptr),nullptr);
		if(!temp_window) throw Runtime_Exception("Couldn't create a temporary window.");
		defer[&]{DestroyWindow(temp_window);};

		HDC temp_dc = GetDC(temp_window);
		if(!temp_dc) throw Runtime_Exception("Couldn't get a temporary DC.");
		defer[&]{ReleaseDC(temp_window,temp_dc);};

		PIXELFORMATDESCRIPTOR temp_format = {};
		temp_format.nSize = sizeof(temp_format);
		temp_format.nVersion = 1;
		temp_format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		temp_format.iPixelType = PFD_TYPE_RGBA;
		temp_format.cColorBits = 32;
		temp_format.cAlphaBits = 8;
		temp_format.cDepthBits = 24;

		int temp_format_index = ChoosePixelFormat(temp_dc,&temp_format);
		if(!temp_format_index) throw Runtime_Exception("Couldn't choose temporary pixel format.");
		if(!SetPixelFormat(temp_dc,temp_format_index,&temp_format)) throw Runtime_Exception("Couldn't set temporary pixel format.");

		HGLRC temp_ctx = wglCreateContext(temp_dc);
		if(!temp_ctx) throw Runtime_Exception("Couldn't create temporary OpenGL context.");
		defer[&]{wglDeleteContext(temp_ctx);};

		if(!wglMakeCurrent(temp_dc,temp_ctx)) throw Runtime_Exception("Couldn't make the temporary OpenGL context current.");
		defer[&]{wglMakeCurrent(temp_dc,nullptr);};

		PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
		if(!wglChoosePixelFormatARB) throw Runtime_Exception("'wglChoosePixelFormatARB' extension isn't supported.");

		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
		if(!wglCreateContextAttribsARB) throw Runtime_Exception("'wglCreateContextAttribsARB' extension isn't supported.");

		static constexpr int Format_Attributes[] = {
			WGL_DRAW_TO_WINDOW_ARB,1,
			WGL_SUPPORT_OPENGL_ARB,1,
			WGL_DOUBLE_BUFFER_ARB,1,
			WGL_PIXEL_TYPE_ARB,WGL_TYPE_RGBA_ARB,
			WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
			WGL_COLOR_BITS_ARB,32,
			WGL_ALPHA_BITS_ARB,8,
			WGL_DEPTH_BITS_ARB,24,
			Array_End_Marker
		};

		int pixel_format_index = 0;
		UINT matching_format_count = 0;
		if(!wglChoosePixelFormatARB(data.dc,Format_Attributes,NULL,1,&pixel_format_index,&matching_format_count) || pixel_format_index == 0) {
			throw Runtime_Exception("Couldn't choose pixel format.");
		}

		PIXELFORMATDESCRIPTOR format = {};
		if(!DescribePixelFormat(data.dc,pixel_format_index,sizeof(format),&format)) throw Runtime_Exception("Couldn't describe pixel format.");
		if(!SetPixelFormat(data.dc,pixel_format_index,&format)) throw Runtime_Exception("Couldn't set pixel format.");

#if defined(DEBUG_BUILD)
		{
			static constexpr int Context_Attributes[] = {
				WGL_CONTEXT_MAJOR_VERSION_ARB,4,
				WGL_CONTEXT_MINOR_VERSION_ARB,3,
				WGL_CONTEXT_PROFILE_MASK_ARB,WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				WGL_CONTEXT_FLAGS_ARB,WGL_CONTEXT_DEBUG_BIT_ARB,
				Array_End_Marker
			};
			HGLRC ctx = wglCreateContextAttribsARB(data.dc,nullptr,Context_Attributes);
			if(ctx) return ctx;
			std::cout << "[Warning] Your computer doesn't support WGL_CONTEXT_DEBUG_BIT_ARB." << std::endl;
		}
#endif

		static constexpr int Context_Attributes[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB,4,
			WGL_CONTEXT_MINOR_VERSION_ARB,3,
			WGL_CONTEXT_PROFILE_MASK_ARB,WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			Array_End_Marker
		};
		HGLRC ctx = wglCreateContextAttribsARB(data.dc,nullptr,Context_Attributes);
		if(!ctx) throw Runtime_Exception("Your computer doesn't support OpenGL 4.3 core.");
		return ctx;
	}

	void Platform::create_main_window(const char* title,std::uint32_t client_width,std::uint32_t client_height) {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		WNDCLASSEXA wc = {};
		wc.cbSize = sizeof(wc);
		wc.lpszClassName = Window_Class_Name;
		wc.hInstance = GetModuleHandleA(nullptr);
		wc.hCursor = LoadCursorA(nullptr,IDC_ARROW);
		wc.hIcon = LoadIconA(nullptr,IDI_APPLICATION);
		wc.hIconSm = wc.hIcon;
		wc.lpfnWndProc = &core::window_proc;
		wc.cbWndExtra = sizeof(Platform_Windows_Data*) + sizeof(LONG_PTR);
		if(!RegisterClassExA(&wc)) throw Runtime_Exception("Couldn't register window class.");

		data.window_client_dims = {client_width,client_height};
		RECT client_rect = {0,0,LONG(client_width),LONG(client_height)};
		if(!AdjustWindowRect(&client_rect,Window_Flags,FALSE)) throw Runtime_Exception("Couldn't adjust client rect dimensions.");
		client_width = std::uint32_t(client_rect.right - client_rect.left);
		client_height = std::uint32_t(client_rect.bottom - client_rect.top);

		data.window = CreateWindowExA(0,Window_Class_Name,title,Window_Flags,CW_USEDEFAULT,CW_USEDEFAULT,int(client_width),int(client_height),HWND_DESKTOP,nullptr,GetModuleHandleA(nullptr),nullptr);
		if(!data.window) throw Runtime_Exception("Couldn't create a window.");
		data.window_closed = false;
		
		data.dc = GetDC(data.window);
		if(!data.dc) throw Runtime_Exception("Couldn't get window DC.");

		SetLastError(0);
		if(!SetWindowLongPtrA(data.window,0,reinterpret_cast<LONG_PTR>(&data))) {
			if(GetLastError() != 0) throw Runtime_Exception("Couldn't set window user data.");
		}

		data.ctx = core::create_opengl_context(data);
		if(!wglMakeCurrent(data.dc,data.ctx)) throw Runtime_Exception("Couldn't make the OpenGL context current.");

		HMODULE opengl32_lib = LoadLibraryA("OpenGL32.dll");
		if(!opengl32_lib) throw Runtime_Exception("Couldn't load OpenGL32.dll.");
		defer[&]{FreeLibrary(opengl32_lib);};
		
#define OPENGL_LOAD_FUNC(TYPE,NAME) \
	do {\
		NAME = reinterpret_cast<TYPE>(wglGetProcAddress(#NAME));\
		if(!NAME) {\
			NAME = reinterpret_cast<TYPE>(GetProcAddress(opengl32_lib,#NAME));\
			if(!NAME) throw Runtime_Exception("Couldn't load OpenGL function '" #NAME "'.");\
		}\
	}\
	while(false);

OPENGL_FUNC_LIST(OPENGL_LOAD_FUNC)
#undef OPENGL_LOAD_FUNC
	}

	bool Platform::window_closed() noexcept {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		return data.window_closed;
	}

	bool Platform::window_resized() noexcept {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		return data.window_resized;
	}

	Dimensions Platform::window_client_dimensions() noexcept {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		return data.window_client_dims;
	}

	void Platform::process_events() {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		std::memset(data.was_key_pressed_statuses,0,sizeof(data.was_key_pressed_statuses));
		data.window_resized = false;
		MSG msg = {};
		while(PeekMessageA(&msg,nullptr,0,0,PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	void Platform::swap_window_buffers() {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		SwapBuffers(data.dc);
	}

	void Platform::error_message_box(const char* title) {
		MessageBoxA(nullptr,title,"Error!",MB_OK | MB_SYSTEMMODAL | MB_ICONERROR);
	}

	bool Platform::is_key_down(Keycode code) const noexcept {
		const Platform_Windows_Data& data = *std::launder(reinterpret_cast<const Platform_Windows_Data*>(data_buffer));
		return data.key_down_statuses[std::size_t(code)];
	}

	bool Platform::was_key_pressed(Keycode code) const noexcept {
		const Platform_Windows_Data& data = *std::launder(reinterpret_cast<const Platform_Windows_Data*>(data_buffer));
		return data.was_key_pressed_statuses[std::size_t(code)];
	}

	Point Platform::mouse_position() const noexcept {
		const Platform_Windows_Data& data = *std::launder(reinterpret_cast<const Platform_Windows_Data*>(data_buffer));
		return data.mouse_position;
	}

	Renderer Platform::create_renderer() {
		return Renderer(this);
	}
}
