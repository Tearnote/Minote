// Minote - minorender.c

#include "minorender.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <string.h>

#include "linmath/linmath.h"
#include "render.h"
#include "log.h"
#include "queue.h"
#include "state.h"
#include "window.h"
#include "mino.h"

#define MINO_SIZE 12 // Size of the mino in pixels
#define INSTANCE_LIMIT 256 // More minos than that will be ignored

static GLuint program = 0;
static GLuint vertexAttrs = 0;
static GLuint vertexBuffer = 0;
static GLuint instanceBuffer = 0;
static GLint projectionAttr = 0;
static GLfloat vertexData[] = {
	0, 0,
	0, MINO_SIZE,
	MINO_SIZE, 0,
	MINO_SIZE, MINO_SIZE
};

// Rendering-ready representation of a mino
typedef struct {
	GLfloat x, y;
	GLfloat r, g, b, a;
} minoInstance;
queue* minoQueue = NULL;

// Most of the wordy OpenGL state wrangling goes here
// I hope I'll never have to edit this
void initMinoRenderer(void) {
	const GLchar vertSrc[] = {
		#include "mino.vert"
	, 0x00};
	const GLchar fragSrc[] = {
		#include "mino.frag"
	, 0x00};
	program = createProgram(vertSrc, fragSrc);
	if(program == 0)
		logError("Failed to initialize mino renderer");
	projectionAttr = glGetUniformLocation(program, "projection");
	glGenVertexArrays(1, &vertexAttrs);
	glGenBuffers(1, &vertexBuffer);
	glGenBuffers(1, &instanceBuffer);
	glBindVertexArray(vertexAttrs);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, INSTANCE_LIMIT * sizeof(minoInstance), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), 0);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	minoQueue = createQueue(sizeof(minoInstance));
}

void cleanupMinoRenderer(void) {
	destroyQueue(minoQueue);
	minoQueue = NULL;
	destroyProgram(program);
	program = 0;
}

void queueMinoPlayfield(mino field[PLAYFIELD_H][PLAYFIELD_W]) {
	// Center the playfield
	int xOffset = renderWidth/2 - PLAYFIELD_W/2*MINO_SIZE;
	int yOffset = renderHeight/2 - PLAYFIELD_H/2*MINO_SIZE;
	
	for(int y = 0; y < PLAYFIELD_H; y++)
	for(int x = 0; x < PLAYFIELD_W; x++) {
		mino minoType = field[y][x];
		//if(minoType == MinoNone) continue; // Commenting this out gives a temporary black background
		minoInstance* newInstance = produceQueueItem(minoQueue);
		newInstance->x = (GLfloat)(x * MINO_SIZE + xOffset);
		newInstance->y = (GLfloat)(y * MINO_SIZE + yOffset);
		memcpy(&newInstance->r, &minoColors[minoType], sizeof(vec4));
		newInstance->r = minoColors[minoType][0] / 5;
		newInstance->g = minoColors[minoType][1] / 5;
		newInstance->b = minoColors[minoType][2] / 5;
		newInstance->a = minoColors[minoType][3];
		
	}
}

void queueMinoPlayer(playerState* player) {
	int xOffset = 0;
	int yOffset = 0;
	
	// Player piece
	if(player->exists) {
		xOffset =  renderWidth/2 - (PLAYFIELD_W/2 - player->x) * MINO_SIZE;
		yOffset = renderHeight/2 - (PLAYFIELD_H/2 - player->y) * MINO_SIZE;
		
		for(int i = 0; i < MINOS_PER_PIECE; i++) {
			coord minoCoord = rs[player->type][player->rotation][i];
			minoInstance* newInstance = produceQueueItem(minoQueue);
			newInstance->x = (GLfloat)(minoCoord.x * MINO_SIZE + xOffset);
			newInstance->y = (GLfloat)(minoCoord.y * MINO_SIZE + yOffset);
			memcpy(&newInstance->r, &minoColors[player->type], sizeof(vec4));
		}
	}
	
	// Preview piece
	xOffset =  renderWidth/2 - (PIECE_BOX/2) * MINO_SIZE;
	yOffset = renderHeight/2 - (PLAYFIELD_H/2 + PIECE_BOX+1) * MINO_SIZE;
	
	for(int i = 0; i < MINOS_PER_PIECE; i++) {
		coord minoCoord = rs[player->preview][0][i];
		minoInstance* newInstance = produceQueueItem(minoQueue);
		newInstance->x = (GLfloat)(minoCoord.x * MINO_SIZE + xOffset);
		newInstance->y = (GLfloat)(minoCoord.y * MINO_SIZE + yOffset);
		memcpy(&newInstance->r, &minoColors[player->preview], sizeof(vec4));
	}
}

void renderMino(void) {
	glUseProgram(program);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, &projection[0][0]);
	glBindVertexArray(vertexAttrs);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
	                (GLsizeiptr)((minoQueue->count > INSTANCE_LIMIT ? INSTANCE_LIMIT : minoQueue->count) * sizeof(minoInstance)),
	                minoQueue->buffer);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)minoQueue->count); // Magic happens here
	glBindVertexArray(0);
	clearQueue(minoQueue);
}