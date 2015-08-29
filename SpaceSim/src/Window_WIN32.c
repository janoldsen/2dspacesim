#ifdef _WIN32
#include "Window.h"
#include <Windows.h>
#include <stdio.h>


LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	switch (msg)
	{
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
};


WindowHandle createWindow(int width, int height)
{

	HINSTANCE currentInstance = GetModuleHandleA(NULL);

	WNDCLASSEX wndClass;
	memset(&wndClass, 0, sizeof(wndClass));
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_OWNDC;
	wndClass.lpfnWndProc = wndProc;
	wndClass.hInstance = currentInstance;
	wndClass.lpszClassName = L"main";
		
	RegisterClassEx(&wndClass);

	HWND window = CreateWindowEx(
		WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		L"main",
		L"SpaceSim",
		WS_CAPTION | WS_SYSMENU, //style
		CW_USEDEFAULT, //posX
		CW_USEDEFAULT, //posY
		width,
		height,
		NULL, //parent
		NULL, //menu,
		currentInstance,
		NULL);
	
	ShowWindow(window, SW_SHOWDEFAULT);

	return (WindowHandle)window;
}



void updateWindow(WindowHandle handle)
{
	MSG msg;
	while (PeekMessage(&msg, handle, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
}
#endif