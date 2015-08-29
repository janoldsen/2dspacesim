#ifdef _WIN32
#include "Graphics.h"
#include <Windows.h>
#include <gl\GL.h>
#include <stdio.h>
#include "GL\glext.h"
//#include "GL\glxext.h"
#include "GL\wglext.h"


PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
PFNGLCLEARBUFFERFVPROC glClearBufferfv;

static void APIENTRY debugMessageCallback(GLenum _source, GLenum _type, GLuint id, GLenum _severity, GLsizei length, const GLchar* message, const void* userParam)
{

	char* source;
	switch (_source)
	{
	case GL_DEBUG_SOURCE_API: source = "API"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: source = "shader"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: source = "window"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY: source = "3rd party"; break;
	case GL_DEBUG_SOURCE_APPLICATION: source = "app"; break;
	default: source = "other";
	}
		

	
	char* type;
	switch (_type)
	{
	case GL_DEBUG_TYPE_ERROR: type = "error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type = "deprecated"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type = "undefined behaviour"; break;
	case GL_DEBUG_TYPE_PERFORMANCE: type = "performance"; break;
	case GL_DEBUG_TYPE_PORTABILITY: type = "portability"; break;
	case GL_DEBUG_TYPE_MARKER: type = "marker"; break;
	case GL_DEBUG_TYPE_OTHER: type = "other"; break;
	default: type = "?";
	}

	char* severity;
	switch (_severity)
	{
	case GL_DEBUG_SEVERITY_LOW: severity = "LOW"; break;
	case GL_DEBUG_SEVERITY_MEDIUM: severity = "MEDIUM"; break;
	case GL_DEBUG_SEVERITY_HIGH: severity = "HIGH"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: severity = "NOTIFICATION"; break;
	default: severity = "";
	}
		



	printf("GLDEBUG(%s) %s error: %s (%s)\n", severity, source, message, type);
}


void initGraphics(int width, int height, WindowHandle window)
{
	HWND hwnd = (HWND)window;
	HDC dc = GetDC(hwnd);


	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW |
		LPD_SUPPORT_OPENGL |
		PFD_GENERIC_ACCELERATED |
		PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cDepthBits = 32;

	int iPixelFromat = ChoosePixelFormat(dc, &pfd);
	SetPixelFormat(dc, iPixelFromat, &pfd);

	//create temporary context
	HGLRC tempContext = wglCreateContext(dc);
	wglMakeCurrent(dc, tempContext);


	//load extensions
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)
		wglGetProcAddress("wglGetExtensionsStringARB");
	const char * extension_string = wglGetExtensionsStringARB(dc);

	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)
		wglGetProcAddress("wglCreateContextAttribsARB");
	glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)
		wglGetProcAddress("glDebugMessageCallback");
	glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)
		wglGetProcAddress("glClearBufferfv");

	// make real one
	{
		int contextFlags = 0;
#ifdef _DEBUG
		contextFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

		int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			WGL_CONTEXT_FLAGS_ARB, contextFlags,
			0
		};
		HGLRC renderContext = wglCreateContextAttribsARB(dc, NULL, attribs);
		wglMakeCurrent(dc, renderContext);
	}
	//destroy temp context
	wglDeleteContext(tempContext);
	

	//add debug functionality
#ifdef _DEBUG
	glDebugMessageCallback(debugMessageCallback, NULL);
#endif


	glViewport(0, 0, width, height);
}

void render()
{
	float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	float one = 1.0f;
	glClearBufferfv(GL_COLOR, 0, color);
	glClearBufferfv(GL_DEPTH, 0, &one);
}


void present(WindowHandle window)
{
	HDC dc = GetDC((HWND)window);
	SwapBuffers(dc);
}
#endif