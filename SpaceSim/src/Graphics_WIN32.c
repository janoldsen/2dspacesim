#ifdef _WIN32
#include "Graphics.h"
#include <Windows.h>
#include <gl\GL.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "GL\glext.h"
//#include "GL\glxext.h"
#include "GL\wglext.h"


PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl;
PFNGLCLEARBUFFERFVPROC glClearBufferfv;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM2UIVPROC glUniform2uiv;

static GLuint g_program;

static loadExtensions()
{

	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
	glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)wglGetProcAddress("glDebugMessageControl");
	glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)wglGetProcAddress("glClearBufferfv");
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog =  (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
	glUniform2uiv = (PFNGLUNIFORM2UIVPROC)wglGetProcAddress("glUniform2uiv");
}

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

static GLuint compileShader(GLenum type, char* src)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	int isCompiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (!isCompiled)
	{
		char error[512];

		glGetShaderInfoLog(shader, 512, NULL, error);

		printf("Shader Error: %s", error);
		glDeleteShader(shader); 

		assert("shaderCompilationFailed" && 0);
		
	}

	return shader;
}


static void packDestruction(int* unpacked, unsigned int* packed)
{
	for (int j = 0; j < 8; ++j)
	{
		for (int i = 0; i < 8; ++i)
		{
			int idx = j / 4;
			int bit = (j - idx * 4) * 8 + i;
			packed[idx] |= unpacked[i + 8 * j] << bit;
		}
	}
}

static void packColors(int* unpacked, unsigned int* packed)
{
	for (int j = 0; j < 8; ++j)
	{
		for (int i = 0; i < 8; ++i)
		{
			int shift = (j * 8 + i) * 3;
			int idx = shift / 32;
			if (shift + 3 > 32)
			{
				packed[idx] |= unpacked[j * 8 + i] << (shift % 32);
				packed[idx+1] |= unpacked[j*8 +i] << 
			}
			else
			{
				packed[idx] |= unpacked[i + 8 * j] << (shift%32);
			}
		}
	}
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

	loadExtensions();


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
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
#endif


	glViewport(0, 0, width, height);

	char buffer[1024] = { 0 };
	FILE* file = fopen("data/shader/test.vs", "r");
	fread(buffer, sizeof(char), 1024, file);

	const GLchar* source = buffer;
	GLuint vs = compileShader(GL_VERTEX_SHADER, buffer);
	fclose(file);

	file = fopen("data/shader/test.fs", "r");
	memset(buffer, 0, sizeof(buffer));
	fread(buffer, sizeof(char), 1024, file);

	GLuint fs = compileShader(GL_FRAGMENT_SHADER, buffer);
	fclose(file);
	
	


	g_program = glCreateProgram();
	glAttachShader(g_program, vs);
	glAttachShader(g_program, fs);
	glLinkProgram(g_program);

	glDeleteShader(vs);
	glDeleteShader(fs);


	GLuint vertexArrayObject;
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);


	int destruction[] = {
		1,1,1,1,1,1,1,0,
		1,1,1,1,1,1,1,0,
		1,1,1,1,1,0,0,0,
		1,1,1,1,1,1,1,0,
		0,0,1,1,1,0,0,0,
		0,0,0,1,1,1,0,0,
		0,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1
	};

	unsigned int _destruction[2] = { 0,0 };

	packDestruction(destruction, _destruction);

	glUseProgram(g_program);
	glUniform2uiv(5, 1, _destruction);
	glUniform2f(0, 800.0f, 600.0f);


}

void render()
{
	float color[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float one = 1.0f;
	glClearBufferfv(GL_COLOR, 0, color);
	glClearBufferfv(GL_DEPTH, 0, &one);



	glUseProgram(g_program);

	

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}


void present(WindowHandle window)
{
	HDC dc = GetDC((HWND)window);
	SwapBuffers(dc);
}
#endif