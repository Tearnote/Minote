#include "render.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "linmath.h"
#include "log.h"
#include "window.h"
#include "thread.h"
#include "state.h"

#define destroyShader glDeleteShader
#define destroyProgram glDeleteProgram

thread rendererThreadID = 0;
static state* gameSnap = NULL;

static mat4x4 projection = {};

static GLuint minoProgram = 0;
static GLuint minoVAO = 0;
static GLuint minoVBO = 0;
static GLuint minoInstanceVBO = 0;
static GLint minoUniform = 0;
static GLfloat minoVertices[] = {
	0, 0,
	0, 24,
	24, 0,
	24, 24
};
static GLfloat minoInstance[] = {
	36, 24, 1, 0, 0, 1
};

static GLuint createShader(const GLchar* source, GLenum type) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	GLint compileStatus = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if(compileStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		logError("Failed to compile shader: %s", infoLog);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint createProgram(const GLchar* vertexShaderSrc, const GLchar* fragmentShaderSrc) {
	GLuint vertexShader = createShader(vertexShaderSrc, GL_VERTEX_SHADER);
	if(vertexShader == 0) return 0;
	GLuint fragmentShader = createShader(fragmentShaderSrc, GL_FRAGMENT_SHADER);
	if(fragmentShader == 0) {
		destroyShader(vertexShader);
		return 0;
	}
	
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	GLint linkStatus = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if(linkStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		logError("Failed to link program: %s", infoLog);
		glDeleteProgram(program);
		program = 0;
	}
	destroyShader(fragmentShader);
	destroyShader(vertexShader);
	return program;
}

static void renderFrame(void) {
	lockMutex(&stateMutex);
	memcpy(gameSnap, game, sizeof(state));
	unlockMutex(&stateMutex);
	
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glUseProgram(minoProgram);
	glUniformMatrix4fv(minoUniform, 1, GL_FALSE, &projection[0][0]);
	glBindVertexArray(minoVAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
	glBindVertexArray(0);
}

static void cleanupRenderer(void) {
	destroyProgram(minoProgram);
	free(gameSnap);
	glfwMakeContextCurrent(NULL); // glfwTerminate() hangs if other threads have a current context
}

static void initRenderer(void) {
	glfwMakeContextCurrent(window);
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { // Not possible to get an error message?
		logCrit("Failed to initialize GLAD");
		cleanupRenderer();
		exit(1);
	}
	glfwSwapInterval(1); // Enable vsync
	
	mat4x4_ortho(projection, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, -1.0f, 1.0f);
	
	const GLchar minoVertSrc[] = {
		#include "mino.vert"
	, 0x00};
	const GLchar minoFragSrc[] = {
		#include "mino.frag"
	, 0x00};
	minoProgram = createProgram(minoVertSrc, minoFragSrc);
	if(minoProgram == 0)
		logError("Failed to initialize rendering pipeline");
	minoUniform = glGetUniformLocation(minoProgram, "projection");
	glGenBuffers(1, &minoInstanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, minoInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(minoInstance), minoInstance, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenVertexArrays(1, &minoVAO);
	glGenBuffers(1, &minoVBO);
	glBindVertexArray(minoVAO);
	glBindBuffer(GL_ARRAY_BUFFER, minoVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(minoVertices), minoVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, minoInstanceVBO);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), 0);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	gameSnap = malloc(sizeof(state));
	
	logInfo("OpenGL renderer initialized");
}

void* rendererThread(void* param) {
	(void)param;
	
	initRenderer();
	
	while(isRunning()) {
		renderFrame();
		glfwSwapBuffers(window);
	}
	
	cleanupRenderer();
	return NULL;
}