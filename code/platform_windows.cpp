#include <iostream>
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <Windows.h>
#include "defer.hpp"
#include "opengl.hpp"
#include "wglext.h"
#include "platform.hpp"
#include "exceptions.hpp"

namespace core {
	static constexpr const char* Window_Class_Name = "CustomWindowClass";
	static constexpr DWORD Window_Flags = WS_BORDER | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX | WS_VISIBLE;
	static constexpr int Array_End_Marker = 0;

	struct Platform_Windows_Data {
		HWND window;
		HDC dc;
		HGLRC ctx;
		bool window_closed;
		bool window_resized;
		Dimensions window_client_dims;
		bool window_class_initialized;
	};

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

		int format_attributes[] = {
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
		if(!wglChoosePixelFormatARB(data.dc,format_attributes,NULL,1,&pixel_format_index,&matching_format_count) || pixel_format_index == 0) {
			throw Runtime_Exception("Couldn't choose pixel format.");
		}

		PIXELFORMATDESCRIPTOR format = {};
		if(!DescribePixelFormat(data.dc,pixel_format_index,sizeof(format),&format)) throw Runtime_Exception("Couldn't describe pixel format.");
		if(!SetPixelFormat(data.dc,pixel_format_index,&format)) throw Runtime_Exception("Couldn't set pixel format.");

		int context_attributes[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB,4,
			WGL_CONTEXT_MINOR_VERSION_ARB,2,
			WGL_CONTEXT_PROFILE_MASK_ARB,WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			Array_End_Marker
		};
		return wglCreateContextAttribsARB(data.dc,nullptr,context_attributes);
	}

	void Platform::create_main_window(const char* title,std::uint32_t width,std::uint32_t height) {
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

		data.window_client_dims = {width,height};
		RECT client_rect = {0,0,LONG(width),LONG(height)};
		if(!AdjustWindowRect(&client_rect,Window_Flags,FALSE)) throw Runtime_Exception("Couldn't adjust client rect dimensions.");
		width = std::uint32_t(client_rect.right - client_rect.left);
		height = std::uint32_t(client_rect.bottom - client_rect.top);

		data.window = CreateWindowExA(0,Window_Class_Name,title,Window_Flags,CW_USEDEFAULT,CW_USEDEFAULT,int(width),int(height),HWND_DESKTOP,nullptr,GetModuleHandleA(nullptr),nullptr);
		if(!data.window) throw Runtime_Exception("Couldn't create a window.");
		data.window_closed = false;
		
		data.dc = GetDC(data.window);
		if(!data.dc) throw Runtime_Exception("Couldn't get window DC.");

		SetLastError(0);
		if(!SetWindowLongPtrA(data.window,0,reinterpret_cast<LONG_PTR>(&data))) {
			if(GetLastError() != 0) throw Runtime_Exception("Couldn't set window user data.");
		}

		data.ctx = core::create_opengl_context(data);
		if(!data.ctx) throw Runtime_Exception("Couldn't create OpenGL context.");
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
}
