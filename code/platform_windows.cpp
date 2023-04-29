#include <iostream>
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <Windows.h>
#include "platform.hpp"
#include "exceptions.hpp"

namespace core {
	static constexpr const char* Window_Class_Name = "CustomWindowClass";
	static constexpr DWORD Window_Flags = WS_BORDER | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;

	struct Platform_Windows_Data {
		HWND window;
		HDC dc;
		HGLRC ctx;
		bool window_closed;
		bool window_class_initialized;
	};

	static LRESULT CALLBACK window_proc(HWND window,UINT msg,WPARAM wparam,LPARAM lparam) {
		switch(msg) {
			case WM_CLOSE: {
				Platform_Windows_Data* data = reinterpret_cast<Platform_Windows_Data*>(GetWindowLongPtrA(window,0));
				data->window_closed = true;
				return 0;
			}
		}
		return DefWindowProcA(window,msg,wparam,lparam);
	}

	Platform::Platform() noexcept : data_buffer() {
		static_assert(sizeof(Platform_Windows_Data) <= sizeof(Platform::data_buffer));
		SetConsoleOutputCP(CP_UTF8);
		new(data_buffer) Platform_Windows_Data();
	}

	Platform::~Platform() {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		DestroyWindow(data.window);
	}

	void Platform::create_main_window(std::string_view title,std::size_t width,std::size_t height) {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		if(!data.window_class_initialized) {
			WNDCLASSEXA wc = {};
			wc.cbSize = sizeof(wc);
			wc.lpszClassName = Window_Class_Name;
			wc.hInstance = GetModuleHandleA(nullptr);
			wc.hIcon = LoadIconA(nullptr,IDI_APPLICATION);
			wc.hIconSm = wc.hIcon;
			wc.lpfnWndProc = &core::window_proc;
			wc.cbWndExtra = sizeof(Platform_Windows_Data*) + sizeof(LONG_PTR);

			if(!RegisterClassExA(&wc)) throw Runtime_Exception("Couldn't register window class.");
			data.window_class_initialized = true;
		}

		RECT client_rect = {0,0,LONG(width),LONG(height)};
		if(!AdjustWindowRect(&client_rect,Window_Flags,false)) throw Runtime_Exception("Couldn't adjust client rect dimensions.");
		width = std::size_t(client_rect.right - client_rect.left);
		height = std::size_t(client_rect.bottom - client_rect.top);

		data.window = CreateWindowExA(0,Window_Class_Name,title.data(),Window_Flags,CW_USEDEFAULT,CW_USEDEFAULT,int(width),int(height),HWND_DESKTOP,nullptr,GetModuleHandleA(nullptr),nullptr);
		if(!data.window) throw Runtime_Exception("Couldn't create a window.");
		data.window_closed = false;

		SetLastError(0);
		if(!SetWindowLongPtrA(data.window,0,reinterpret_cast<LONG_PTR>(&data))) {
			if(GetLastError() != 0) throw Runtime_Exception("Couldn't set window user data.");
		}
	}

	bool Platform::window_closed() noexcept {
		Platform_Windows_Data& data = *std::launder(reinterpret_cast<Platform_Windows_Data*>(data_buffer));
		return data.window_closed;
	}

	void Platform::process_events() {
		MSG msg = {};
		while(PeekMessageA(&msg,nullptr,0,0,PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	void Platform::swap_buffers() {

	}

	void Platform::error_message_box(std::string_view title) {
		MessageBoxA(nullptr,title.data(),"Error!",MB_OK | MB_ICONERROR);
	}
}
