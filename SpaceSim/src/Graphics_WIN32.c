#ifdef _WIN32
#include "Graphics.h"
#include "FileSystem.h"
#include "JobSystem.h"
#include <Windows.h>
#include <gl\GL.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../bin/data/shader/ShaderDefinitions.h"
#include "Math.h"
#include "Graphics_Extension_WIN32.c"

#define VERSION_DIRECTIVE "#version 430 core\n"
#define LINE_DIRECTIVE "#line 1\n"

static GLuint g_program;
static GLuint g_transformBuffer;



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




	printf("\nGLDEBUG(%s, %s):\n%s (%s)\n", severity, source, message, type);
}

static GLuint compileShader(GLenum type, char* src, char* header)
{
	GLuint shader = glCreateShader(type);
	if (header == 0)
	{
		glShaderSource(shader, 1, &src, NULL);
	}
	else
	{
		char* srcs[] = { VERSION_DIRECTIVE, header, LINE_DIRECTIVE, src };
		glShaderSource(shader, 4, srcs, NULL);
	}

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
			int unpackedIdx = j * 8 + i;
			int totalShift = unpackedIdx * 3;
			int packedIdx = totalShift / 32;
			int shift = totalShift % 32;


			packed[packedIdx] |= (unpacked[unpackedIdx] << shift);

			if (shift >= 30)
				packed[packedIdx + 1] |= (unpacked[unpackedIdx] >> (32 - shift));
		}
	}
}

static void unPackColors(unsigned int* packed, int* unpacked)
{
	for (int j = 0; j < 8; ++j)
	{
		for (int i = 0; i < 8; ++i)
		{
			int unpackedIdx = j * 8 + i;
			int totalShift = unpackedIdx * 3;
			int packedIdx = totalShift / 32;
			int shift = totalShift % 32;


			unpacked[unpackedIdx] |= (packed[packedIdx] >> shift) & (1 << 3) - 1;

			if (shift >= 30)
				unpacked[unpackedIdx] |= (packed[packedIdx + 1] & (1 << (shift - 32 + 3)) - 1) << (32 - shift);
		}
	}
}



void setUpShips()
{
	int destruction[2][64] = {
		{
		1,1,1,1,1,1,1,1,
		1,0,1,1,1,1,0,1,
		1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,
		1,0,1,1,1,1,0,1,
		1,1,1,1,1,1,1,1
		},
		{
		1,1,1,1,1,1,1,0,
		1,1,1,1,1,1,0,1,
		1,1,1,1,1,0,1,1,
		1,1,1,1,0,1,1,1,
		1,1,1,0,1,1,1,1,
		1,1,0,1,1,1,1,1,
		1,0,1,1,1,1,1,1,
		0,1,1,1,1,1,1,1
		},
	};

	unsigned int _destruction[2][2] = { 0,0 };

	for (int i = 0; i < 2; ++i)
		packDestruction(destruction[i], _destruction[i]);


	int colors[9][64] = {
		{
		0,0,0,0,0,0,0,0,
		0,1,1,1,1,1,1,1,
		0,1,2,2,2,2,2,2,
		0,1,2,3,3,3,3,3,
		0,1,2,3,4,4,4,4,
		0,1,2,3,4,5,5,5,
		0,1,2,3,4,5,6,6,
		0,1,2,3,4,5,6,7
		},
		{
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,6,
		0,1,2,3,4,5,5,5,
		0,1,2,3,4,4,4,4,
		0,1,2,3,3,3,3,3,
		0,1,2,2,2,2,2,2,
		0,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,0
		},
		{
		7,6,5,4,3,2,1,0,
		6,6,5,4,3,2,1,0,
		5,5,5,4,3,2,1,0,
		4,4,4,4,3,2,1,0,
		3,3,3,3,3,2,1,0,
		2,2,2,2,2,2,1,0,
		1,1,1,1,1,1,1,0,
		0,0,0,0,0,0,0,0
		},
		{
		0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,0,
		2,2,2,2,2,2,1,0,
		3,3,3,3,3,2,1,0,
		4,4,4,4,3,2,1,0,
		5,5,5,4,3,2,1,0,
		6,6,5,4,3,2,1,0,
		7,6,5,4,3,2,1,0
		},
		{
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7
		},
		{
		7,6,5,4,3,2,1,0,
		7,6,5,4,3,2,1,0,
		7,6,5,4,3,2,1,0,
		7,6,5,4,3,2,1,0,
		7,6,5,4,3,2,1,0,
		7,6,5,4,3,2,1,0,
		7,6,5,4,3,2,1,0,
		7,6,5,4,3,2,1,0,
		},
		{
		0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,
		3,3,3,3,3,3,3,3,
		4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7
		},
		{
		7,7,7,7,7,7,7,7,
		6,6,6,6,6,6,6,6,
		5,5,5,5,5,5,5,5,
		4,4,4,4,4,4,4,4,
		3,3,3,3,3,3,3,3,
		2,2,2,2,2,2,2,2,
		1,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,0
		},
		{
		0,1,2,3,4,5,6,7,
		7,0,1,2,3,4,5,6,
		6,7,0,1,2,3,4,5,
		5,6,7,0,1,2,3,4,
		4,5,6,7,0,1,2,3,
		3,4,5,6,7,0,1,2,
		2,3,4,5,6,7,0,1,
		1,2,3,4,5,6,7,0
		},
	};

	unsigned int _colors[9][6] = { 0 };

	for (int i = 0; i < 9; ++i)
	{
		packColors(colors[i], _colors[i]);
	}

	float colorPalette[2][8][4] =
	{
	{
		{ 0.8f, 0.8f, 0.8f },
		{ 0.7f, 0.7f, 0.7f },
		{ 0.6f, 0.6f, 0.6f },
		{ 0.5f, 0.5f, 0.5f },
		{ 0.4f, 0.4f, 0.4f },
		{ 0.3f, 0.3f, 0.3f },
		{ 0.2f, 0.2f, 0.2f },
		{ 0.1f, 0.1f, 0.1f }
	},
	{
		{ 0.99f, 0.0f, 0.0f },
		{ 1.0f, 0.61f, 0.0f },
		{ 0.97f, 0.99f, 0.0f },
		{ 0.235f, 0.94f, 0.05f },
		{ 0.0f, 1.0f, 0.89f },
		{ 0.0f, 0.14f, 1.0f },
		{ 0.77f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 0.61f }
	}
	};


	float angle = 0.3f;
	float transform[2][12] =
	{
	{
		1, 0, -500, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
	},
	{
		1, 0, 500, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
	}
	};


#define size 16
	// set up plates
	struct StaticData
	{
		int pos[2];
		int shipIdx;
		int colorIdx;
	} staticData[2][size*size] = { 0 };
	int destructionIdx[2][size*size] = { 0 };

	for (int k = 0; k < 2; ++k)
	{
		for (int j = 0; j < size; ++j)
		{
			for (int i = 0; i < size; ++i)
			{
				int color = 8;
				if (i == 0 && j == (size - 1))
					color = 0;
				else if (i == 0 && j == 0)
					color = 1;
				else if (i == (size - 1) && j == 0)
					color = 2;
				else if (i == (size - 1) && j == (size - 1))
					color = 3;
				else if (i == 0)
					color = 4;
				else if (i == (size - 1))
					color = 5;
				else if (j == (size - 1))
					color = 6;
				else if (j == 0)
					color = 7;
				else
					destructionIdx[k][j * size + i] = 1;

				staticData[k][j * size + i].pos[0] = i - size / 2;
				staticData[k][j * size + i].pos[1] = j - size / 2;
				staticData[k][j * size + i].shipIdx = k;
				staticData[k][j * size + i].colorIdx = color;
			}
		}
	}



	GLuint buffers[2];
	glGenBuffers(2, buffers);

	GLuint staticBuffer = buffers[0];
	GLuint destructionBuffer = buffers[1];

	//fill buffers
	glBindBuffer(GL_ARRAY_BUFFER, staticBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(staticData), &staticData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, destructionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(destructionIdx), &destructionIdx, GL_DYNAMIC_DRAW);

	//set up attribs
	int offsets[] = { 0,0 };
	size_t strides[] = { sizeof(struct StaticData), sizeof(int) };
	glBindVertexBuffers(0, 2, buffers, offsets, strides);

	glVertexBindingDivisor(0, 1);
	glVertexBindingDivisor(1, 1);

	glVertexAttribIFormat(POS_ATTRIB_LOC, 2, GL_INT, offsetof(struct StaticData, pos));
	glVertexAttribIFormat(SHIP_IDX_ATTRIB_LOC, 1, GL_INT, offsetof(struct StaticData, shipIdx));
	glVertexAttribIFormat(COLOR_IDX_ATTRIB_LOC, 1, GL_INT, offsetof(struct StaticData, colorIdx));
	glVertexAttribIFormat(DESTRUCTION_IDX_ATTRIB_LOC, 1, GL_INT, 0);

	glVertexAttribBinding(POS_ATTRIB_LOC, 0);
	glVertexAttribBinding(SHIP_IDX_ATTRIB_LOC, 0);
	glVertexAttribBinding(COLOR_IDX_ATTRIB_LOC, 0);
	glVertexAttribBinding(DESTRUCTION_IDX_ATTRIB_LOC, 1);

	glEnableVertexAttribArray(POS_ATTRIB_LOC);
	glEnableVertexAttribArray(SHIP_IDX_ATTRIB_LOC);
	glEnableVertexAttribArray(COLOR_IDX_ATTRIB_LOC);
	glEnableVertexAttribArray(DESTRUCTION_IDX_ATTRIB_LOC);





	// set up destruction
	{
		GLuint destructionBuffer;
		glGenBuffers(1, &destructionBuffer);
		glBindBuffer(GL_TEXTURE_BUFFER, destructionBuffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(_destruction), &_destruction, GL_STATIC_DRAW);

		GLuint destructionTexture;
		glGenTextures(1, &destructionTexture);
		glActiveTexture(GL_TEXTURE0 + DESTRUCTION_TEX_BINDING);
		glBindTexture(GL_TEXTURE_BUFFER, destructionTexture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, destructionBuffer);
	}

	// set up color
	{
		GLuint colorBuffer;
		glGenBuffers(1, &colorBuffer);
		glBindBuffer(GL_TEXTURE_BUFFER, colorBuffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(_colors), &_colors, GL_STATIC_DRAW);

		GLuint colorTexture;
		glGenTextures(1, &colorTexture);
		glActiveTexture(GL_TEXTURE0 + COLOR_TEX_BINDING);
		glBindTexture(GL_TEXTURE_BUFFER, colorTexture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, colorBuffer);
	}

	// set up color palette
	{
		GLuint colorPaletteBuffer;
		glGenBuffers(1, &colorPaletteBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, colorPaletteBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(colorPalette), colorPalette[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, COLOR_PALETTE_UNI_BINDING, colorPaletteBuffer);
	}

	// set up ship transforms
	{

		GLuint shipTransforms;
		glGenBuffers(1, &shipTransforms);
		glBindBuffer(GL_UNIFORM_BUFFER, shipTransforms);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(transform), transform, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_UNI_BINDING, shipTransforms);
		g_transformBuffer = shipTransforms;
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

	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

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

	loadExtensions(dc);

	//add debug functionality
#ifdef _DEBUG
	glDebugMessageCallback(debugMessageCallback, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
#endif


	glViewport(0, 0, width, height);


	char header[2048] = { 0 };

	File* pFile;
	pFile = fsCreate("data/shader/shaderDefinitions.h", NULL);
	fsOpenRead(pFile);
	fsRead(pFile, header, sizeof(header));
	fsClose(pFile);

	char src[2048] = { 0 };
	pFile = fsCreate("data/shader/test.vs", NULL);
	fsOpenRead(pFile);
	fsRead(pFile, src, sizeof(src));
	fsClose(pFile);

	GLuint vs = compileShader(GL_VERTEX_SHADER, src, header);

	pFile = fsCreate("data/shader/test.fs", NULL);
	memset(src, 0, sizeof(src));
	fsOpenRead(pFile);
	fsRead(pFile, src, sizeof(src));
	fsClose(pFile);


	GLuint fs = compileShader(GL_FRAGMENT_SHADER, src, header);

	g_program = glCreateProgram();
	glAttachShader(g_program, vs);
	glAttachShader(g_program, fs);
	glLinkProgram(g_program);

	GLint isLinked = 0;
	glGetProgramiv(g_program, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		char error[512];

		glGetProgramInfoLog(g_program, 512, NULL, error);

		printf("Link Error: %s", error);
		glDeleteProgram(g_program);

		assert("ProgramLinkingFailed" && 0);

	}


	GLint test = glGetUniformLocation(g_program, "screensize");

	GLuint vertexArrayObject;
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	glUseProgram(g_program);

	setUpShips();

	//glUniform1uiv(5, 2, _destruction);
	//glUniform1uiv(7, 6, _colors);
	glUniform2f(SCREEN_SIZE_UNI_LOC, 800.0f, 600.0f);


}




void render(double dt)
{
	float color[] = { 0.3f, 0.0f, 0.0f, 1.0f };
	float one = 1.0f;
	glClearBufferfv(GL_COLOR, 0, color);
	glClearBufferfv(GL_DEPTH, 0, &one);

	static float angle = 0.0f;

	angle += (float)(0.1 * dt);
	glBindBuffer(GL_UNIFORM_BUFFER, g_transformBuffer);
	float* transform = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	transform[0] = cosf(angle);
	transform[1] = -sinf(angle);
	transform[4] = sinf(angle);
	transform[5] = cosf(angle);

	transform[12] = cosf(-angle);
	transform[13] = -sinf(-angle);
	transform[16] = sinf(-angle);
	transform[17] = cosf(-angle);


	glUnmapBuffer(GL_UNIFORM_BUFFER);
	//glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_UNI_BINDING, g_transformBuffer);

	//glUseProgram(g_program);

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, size*size * 2);

}


void present(WindowHandle window)
{
	HDC dc = GetDC((HWND)window);
	SwapBuffers(dc);
}
#endif