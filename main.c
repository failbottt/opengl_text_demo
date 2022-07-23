// Purpose
// ---
// This is a simple - at least as simple as I can make it - demo of 
// how to draw text to a screen using Opengl. It's in vanilla C and the
// only dependancies are Freetype and GLFW (for cross-platform Window creation and I/O handling)
//
// I'm on a Linux machine, so to get freetype I ran `apt install libfreetype-dev`
//
// I include GLFW in the source code because I wanted to learn how to statically link against it.
//
// If you downloaded the source code you'll need to compile GLFW for your platform (it's in the /lib directory). 
// After that it _should_ work. However, you might run into issues, and that's part of the "fun" of building native
// programs. You'll just have to work through the problems you run into.
//
// Hight level overview:
// ----
// 1. Using freetype, load the first 128 ASCII characters into memory
// 2. Setup the opengl program
// 3. Loop over each character in the text we want to draw 
//	- lookup the character texture in the Characters array (the one we created in step 1)
//	- Draw it to the screen
//
// Resources: If you're just learning about Opengl here are some useful resources
// ---
// 1. Opengl Superbible: https://www.amazon.com/OpenGL-Superbible-Comprehensive-Tutorial-Reference/dp/0672337479
// 2. The Cherno: https://www.youtube.com/c/TheChernoProject
// 3. An Opengl template by Tsoding: https://github.com/tsoding/opengl-template
//
// Performance:
// ---
// This isn't the most performant way to render text. Right now it loops over each character to be drawn and:
// 1. updates GPU memory
// 2. performs a draw call
//
// A better way would be to create a texture that contains all the text and then make a single draw call. As of JUL22 I'm not sure how to do that yet. Once I figure it out I'll add another example on my website.
//
// Last Note:
// ---
// If you don't have a graphics background learning opengl kind of sucks. But you'll pick things
// up over time, so keep at it. I highly recommend buying the Opengl Superbible. Just reading the first
// couple of chapters gave me more understanding than a couple months worth of trial and error.
//
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "GLFW/glfw3.h"

#include "glextloader.c"

#define SCREEN_WIDTH 900 
#define SCREEN_HEIGHT 600 

uint8_t main_program;
uint8_t vertex, fragment;
unsigned int VAO, VBO;

typedef struct {
	float x;
	float y;
} VEC2;

/// Holds all state information relevant to a character as loaded using FreeType
typedef struct {
	int16_t TextureID; // ID handle of the glyph texture
	VEC2 Size;      // Size of glyph
	VEC2 Bearing;   // Offset from baseline to left/top of glyph
	int16_t Advance;   // Horizontal offset to advance to next glyph
} Character;

Character Characters[128];


void render_text(const char* text, float x, float y, float scale);
char *read_file(const char *file_path);
void check_gl_compile_errors(GLuint shader, char *type);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ed", NULL, NULL);
	if (window == NULL)
	{
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// find memory location of the opengl functions used below
	load_gl_extensions();

	// OpenGL state
	// ------------
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// read shaders
	const char *vert_src = read_file("shader.vert");
	const char *frag_src = read_file("shader.frag");

	// compile vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vert_src, NULL);
	glCompileShader(vertex);
	check_gl_compile_errors(vertex, "VERTEX");

	// compile fragment shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &frag_src, NULL);
	glCompileShader(fragment);
	check_gl_compile_errors(fragment, "FRAGMENT");

	// create program and attach shaders to program
	main_program = glCreateProgram();
	glAttachShader(main_program, vertex);
	glAttachShader(main_program, fragment);
	glLinkProgram(main_program);
	check_gl_compile_errors(main_program, "PROGRAM");

	// clean up shader source and compiled shaders
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	free((void*)vert_src);
	free((void*)frag_src);


	// SET PROJECTION TO 2D
	// All this does is set the coordinate space of the window to 2D
	//									   (x,y)
	// so the bottom left of the screen is (0,0).
	// If you want to to know more about projection here's a pretty good
	// youtube video by The Cherno: https://www.youtube.com/watch?v=xZs6K7VLM7A
	//
	// I got the math from https://github.com/recp/cglm.
	// I didn't want to pull in the entire library for one function,
	// so I just copied it.
	//
	// ORTHO FUNCTION
	float pj[4][4] = {{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},  
		{0.0f, 0.0f, 0.0f, 0.0f},                    \
		{0.0f, 0.0f, 0.0f, 0.0f}};

	float rl, tb, fn;
	float left = 0.0f;
	float right = (float)SCREEN_WIDTH;
	float bottom = 0.0f;
	float top = (float) SCREEN_HEIGHT;
	float nearZ = -1.0f;
	float farZ = 1.0f;

	rl = 1.0f / (right  - left);
	tb = 1.0f / (top    - bottom);
	fn =-1.0f / (farZ - nearZ);

	pj[0][0] = 2.0f * rl;
	pj[1][1] = 2.0f * tb;
	pj[2][2] = 2.0f * fn;
	pj[3][0] =-(right  + left)    * rl;
	pj[3][1] =-(top    + bottom)  * tb;
	pj[3][2] = (farZ + nearZ) * fn;
	pj[3][3] = 1.0f;
	// ORTHO END

	glUseProgram(main_program);
	glUniformMatrix4fv(glGetUniformLocation(main_program, "projection"), 1, GL_FALSE, (float *)pj);
	// SET PROJECTION TO 2D END

	// Use FreeType to generate a texture for ASCII characters 0-128
	// --------
	FT_Library ft;
	// All functions return a value different than 0 whenever an error occurred
	if (FT_Init_FreeType(&ft))
	{
		printf("ERROR::FREETYPE: Could not init FreeType Library\n");
		return -1;
	}

	// load font as face
	FT_Face face;
	if (FT_New_Face(ft, "./external/fonts/Hack-Regular.ttf", 0, &face)) {
		printf("ERROR::FREETYPE: Failed to load font at ./external/fonts/Hack-Regular.ttf\n");
		return -1;
	}
	else {
		FT_Set_Pixel_Sizes(face, 16, 16);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// make textures for ASCII characters 0-128
		for (uint8_t c = 0; c < 128; c++)
		{
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				printf("ERROR::FREETYTPE: Failed to load Glyph\n");
				continue;
			}

			unsigned int texture;
			glGenTextures(1, &texture);

			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_RED,
					face->glyph->bitmap.width,
					face->glyph->bitmap.rows,
					0,
					GL_RED,
					GL_UNSIGNED_BYTE,
					face->glyph->bitmap.buffer
					);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			VEC2 size = {face->glyph->bitmap.width, face->glyph->bitmap.rows};
			VEC2 bearing = {face->glyph->bitmap_left, face->glyph->bitmap_top};
			Character character = {
				texture,
				size,
				bearing,
				(unsigned int)(face->glyph->advance.x)
			};

			Characters[c] = character;
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// configure VAO/VBO for texture quads
	// -----------------------------------
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	char *code = "Good news, everyone!";

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		float text_padding_left = 10.0f;

		// NOTE: remember that the bottom left of the window is (0,0)
		// so to set the location of the text to the top of the window
		// we need to set location to the window height and then subtract.
		float text_y_coordinates = (float)(SCREEN_HEIGHT) - 18;

		render_text(
				code, 
				text_padding_left, 
				text_y_coordinates,
				1.0f
				);

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

// render line of text
// -------------------
void render_text(const char *text, float x, float y, float scale)
{
	glUseProgram(main_program);

	// font color
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;

	glUniform3f(glGetUniformLocation(main_program, "textColor"), r, g, b);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);

	float xpos = 0.0f;
	float ypos = 0.0f;

	for (unsigned int c = 0; c != strlen(text); c++) 
	{
		Character ch = Characters[(int)text[c]];
		xpos = x + ch.Bearing.x * scale;
		ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },            
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }           
		};

		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		x += (ch.Advance >> 6) * scale;
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

char *read_file(const char *file_path)
{
#define SLURP_FILE_PANIC \
  do { \
    fprintf(stderr, "Could not read file `%s`: %s\n", file_path, strerror(errno)); \
    exit(1); \
  } while (0)

  FILE *f = fopen(file_path, "r");
  if (f == NULL) SLURP_FILE_PANIC;
  if (fseek(f, 0, SEEK_END) < 0) SLURP_FILE_PANIC;

  long size = ftell(f);
  if (size < 0) SLURP_FILE_PANIC;

  char *buffer = (char *)malloc(size + 1);
  if (buffer == NULL) SLURP_FILE_PANIC;

  if (fseek(f, 0, SEEK_SET) < 0) SLURP_FILE_PANIC;

  fread(buffer, 1, size, f);
  if (ferror(f) < 0) SLURP_FILE_PANIC;

  buffer[size] = '\0';

  if (fclose(f) < 0) SLURP_FILE_PANIC;

  return buffer;
#undef SLURP_FILE_PANIC
}

void check_gl_compile_errors(GLuint shader, char *type)
{
  GLint success;
  GLchar infoLog[1024];
  if(strcmp(type, "PROGRAM") != 0)
  {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      printf("ERROR::SHADER_COMPILATION_ERROR of type %s: error: %s\n", type, infoLog);
      printf("----------------------------------------\n");
    }
  }
  else
  {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if(!success)
    {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      printf("ERROR::PROGRAM_LINKING of type %s: error: %s\n", type, infoLog);
      printf("----------------------------------------\n");
    }
  }
}
