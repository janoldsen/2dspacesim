#ifdef _WIN32
#include "Window.h"
#include <Windows.h>


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

	WNDCLASSA wndClass = 
	{
		CS_HREDRAW | CS_VREDRAW, // style,
		wndProc,
		0, //extra bytes
		0, //extra bytes instance
		currentInstance,
		NULL, //icon
		NULL, //cursor
		NULL, //background
		NULL, //menu name
		"main" //class name
	};
	
	RegisterClassA(&wndClass);

	
	
	HWND window = CreateWindowA(
		"main",
		"SpaceSim",
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




#endif