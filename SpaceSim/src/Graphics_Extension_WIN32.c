#include <Windows.h>
#include <gl\GL.h>
#include "GL\glext.h"
#include "GL\wglext.h"

PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl;
PFNGLCLEARBUFFERFVPROC glClearBufferfv;
PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced;

PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;


PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;
PFNGLBINDBUFFERBASEPROC glBindBufferBase;
PFNGLBINDVERTEXBUFFERSPROC glBindVertexBuffers;
PFNGLVERTEXBINDINGDIVISORPROC glVertexBindingDivisor;
PFNGLVERTEXATTRIBFORMATPROC glVertexAttribFormat;
PFNGLVERTEXATTRIBIFORMATPROC glVertexAttribIFormat;
PFNGLVERTEXATTRIBBINDINGPROC glVertexAttribBinding;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
PFNGLTEXBUFFERPROC glTexBuffer;
PFNGLACTIVETEXTUREPROC glActiveTexture;

PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM1UIVPROC glUniform1uiv;
PFNGLUNIFORM2UIVPROC glUniform2uiv;

static void loadExtensions(HDC hdc)
{
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)
		wglGetProcAddress("wglGetExtensionsStringARB");
	const char * extension_string = wglGetExtensionsStringARB(hdc);

	glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
	glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)wglGetProcAddress("glDebugMessageControl");
	glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)wglGetProcAddress("glClearBufferfv");
	glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)wglGetProcAddress("glDrawArraysInstanced");

	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");

	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");
	glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase");
	glBindVertexBuffers = (PFNGLBINDVERTEXBUFFERSPROC)wglGetProcAddress("glBindVertexBuffers");
	glVertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC)wglGetProcAddress("glVertexBindingDivisor");
	glVertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)wglGetProcAddress("glVertexAttribFormat");
	glVertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)wglGetProcAddress("glVertexAttribIFormat");
	glVertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC)wglGetProcAddress("glVertexAttribBinding");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)wglGetProcAddress("glVertexAttribDivisor");
	glTexBuffer = (PFNGLTEXBUFFERPROC)wglGetProcAddress("glTexBuffer");
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");

	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
	glUniform1uiv = (PFNGLUNIFORM1UIVPROC)wglGetProcAddress("glUniform1uiv");
	glUniform2uiv = (PFNGLUNIFORM2UIVPROC)wglGetProcAddress("glUniform2uiv");

	

}