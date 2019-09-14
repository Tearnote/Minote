// Minote - textrender.c

#include "textrender.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <unitypes.h>
#include <unistr.h>
#include <unistdio.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "linmath/linmath.h"

#include "font.h"
#include "render.h"
#include "log.h"
#include "queue.h"
#include "util.h"
#include "state.h"

#define VERTEX_LIMIT 8192

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;
static GLint colorAttr = -1;
static GLint pxRangeAttr = -1;

struct textVertex {
	GLfloat x, y, z;
	GLfloat tx, ty;
};

static vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };

// Each font has a separate queue
static queue *textQueue[FontSize] = {};

void initTextRenderer(void)
{
	for (int i = 0; i < FontSize; i++)
		textQueue[i] = createQueue(sizeof(struct textVertex));

	initFonts();

	const GLchar vertSrc[] = {
#include "text.vert"
		, 0x00 };
	const GLchar fragSrc[] = {
#include "text.frag"
		, 0x00 };
	program = createProgram(vertSrc, fragSrc);
	if (program == 0)
		logError("Failed to initialize text renderer");
	cameraAttr = glGetUniformLocation(program, "camera");
	projectionAttr = glGetUniformLocation(program, "projection");
	colorAttr = glGetUniformLocation(program, "color");
	pxRangeAttr = glGetUniformLocation(program, "pxRange");

	glGenBuffers(1, &vertexBuffer);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5,
	                      (GLvoid *)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5,
	                      (GLvoid *)(sizeof(GLfloat) * 3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void cleanupTextRenderer(void)
{
	glDeleteVertexArrays(1, &vao);
	vao = 0;
	glDeleteBuffers(1, &vertexBuffer);
	vertexBuffer = 0;
	destroyProgram(program);
	program = 0;

	cleanupFonts();

	for (int i = 0; i < FontSize; i++) {
		destroyQueue(textQueue[i]);
		textQueue[i] = NULL;
	}
}

static void
queueGlyph(enum fontType font, ucs4_t codepoint, const vec3 position,
           GLfloat size)
{
	if (codepoint >= fonts[font].glyphCount)
		return;

	vec3 adjusted = {};
	int xOffset = fonts[font].glyphs[codepoint].xOffset;
	int yOffset = fonts[font].size - fonts[font].glyphs[codepoint].height
	              - fonts[font].glyphs[codepoint].yOffset;
	adjusted[0] =
		position[0] + (GLfloat)xOffset / (float)fonts[font].size * size;
	adjusted[1] =
		position[1] + (GLfloat)yOffset / (float)fonts[font].size * size;
	adjusted[2] = position[2];

	vec3 bottomLeft = {};
	vec3 bottomRight = {};
	vec3 topLeft = {};
	vec3 topRight = {};

	GLfloat tx1 = (GLfloat)fonts[font].glyphs[codepoint].x;
	GLfloat ty1 = (GLfloat)fonts[font].glyphs[codepoint].y;
	GLfloat tx2 = tx1 + (GLfloat)fonts[font].glyphs[codepoint].width;
	GLfloat ty2 = ty1 + (GLfloat)fonts[font].glyphs[codepoint].height;
	GLfloat w = (tx2 - tx1) / (GLfloat)fonts[font].size * size;
	GLfloat h = (ty2 - ty1) / (GLfloat)fonts[font].size * size;
	tx1 /= (GLfloat)fonts[font].atlasSize;
	ty1 /= (GLfloat)fonts[font].atlasSize;
	tx2 /= (GLfloat)fonts[font].atlasSize;
	ty2 /= (GLfloat)fonts[font].atlasSize;

	bottomLeft[0] = adjusted[0];
	bottomLeft[1] = adjusted[1];
	bottomLeft[2] = adjusted[2];

	bottomRight[0] = adjusted[0] + w;
	bottomRight[1] = adjusted[1];
	bottomRight[2] = adjusted[2];

	topLeft[0] = adjusted[0];
	topLeft[1] = adjusted[1] + h;
	topLeft[2] = adjusted[2];

	topRight[0] = adjusted[0] + w;
	topRight[1] = adjusted[1] + h;
	topRight[2] = adjusted[2];

	struct textVertex *newVertex;
	newVertex = produceQueueItem(textQueue[font]);
	newVertex->x = bottomLeft[0];
	newVertex->y = bottomLeft[1];
	newVertex->z = bottomLeft[2];
	newVertex->tx = tx1;
	newVertex->ty = ty2;
	newVertex = produceQueueItem(textQueue[font]);
	newVertex->x = bottomRight[0];
	newVertex->y = bottomRight[1];
	newVertex->z = bottomRight[2];
	newVertex->tx = tx2;
	newVertex->ty = ty2;
	newVertex = produceQueueItem(textQueue[font]);
	newVertex->x = topRight[0];
	newVertex->y = topRight[1];
	newVertex->z = topRight[2];
	newVertex->tx = tx2;
	newVertex->ty = ty1;

	newVertex = produceQueueItem(textQueue[font]);
	newVertex->x = bottomLeft[0];
	newVertex->y = bottomLeft[1];
	newVertex->z = bottomLeft[2];
	newVertex->tx = tx1;
	newVertex->ty = ty2;
	newVertex = produceQueueItem(textQueue[font]);
	newVertex->x = topRight[0];
	newVertex->y = topRight[1];
	newVertex->z = topRight[2];
	newVertex->tx = tx2;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue[font]);
	newVertex->x = topLeft[0];
	newVertex->y = topLeft[1];
	newVertex->z = topLeft[2];
	newVertex->tx = tx1;
	newVertex->ty = ty1;
}

static void
queueString(enum fontType font, vec3 position, float size, char *fmt, ...)
{
	uint8_t *ustring;
	va_list ap;
	va_start(ap, fmt);
	if (u8_u8_vasprintf(&ustring, (uint8_t *)fmt, ap) == -1) {
		logWarn("Failed to allocate memory for string");
		va_end(ap);
		return;
	}
	va_end(ap);

	vec3 cursor;
	copyArray(cursor, position);
	const uint8_t *iterator = ustring;
	ucs4_t codepoint;
	while (true) {
		iterator = u8_next(&codepoint, iterator);
		if (!iterator)
			break;
		queueGlyph(font, codepoint, cursor, size);
		if (codepoint >= fonts[font].glyphCount)
			continue;
		cursor[0] += (float)fonts[font].glyphs[codepoint].advance
		             / (float)fonts[font].size * size;
	}

	free(ustring);
}

void queueGameplayText(struct game *game)
{
	vec3 position = { 6.5f, 5.0f, 1.0f };
	float size = 1.25f;
	queueString(FontMono, position, size, "%3d", game->level);
	position[1] -= 2.0f;
	queueString(FontMono, position, size, "%d", game->nextLevelstop);
	position[0] = 6.0f;
	position[1] = 13.0f;
	size = 1.0f;
	queueString(FontMono, position, size, "%d", game->score);
	size = 4.0f;
	char grade[2];
	grade[0] = game->gradeString[0];
	grade[1] = '\0';
	if (strlen(game->gradeString) == 1)
		position[0] = 6.85f;
	else
		position[0] = 6.25f;
	position[1] = 15.2f;
	queueString(FontMono, position, size, "%U", grade);
	if (game->gradeString[1]) {
		grade[0] = game->gradeString[1];
		size = 2.0f;
		position[0] += 2.18f;
		position[1] -= 0.3f;
		queueString(FontMono, position, size, "%U", grade);
	}
	position[0] = -10.5f;
	position[1] = 5.0f;
	size = 1.0f;
	queueString(FontMono, position, size, "%2d:%02d:%02d",
	            game->time / SEC / 60,
	            game->time / SEC % 60,
	            game->time / (SEC / 100) % 100);
	if (getState() == AppGameplay && !game->started) {
		position[0] = 6.0f;
		position[1] = 11.5f;
		size = 1.0f;
		queueString(FontSans, position, size,
		            "Press Start/Enter to begin.");
	}
}

void queueReplayText(struct replay *replay)
{
	vec3 position = { -9.5f, 13.0f, 1.0f };
	float size = 1.0f;
	if (replay->playback)
		queueString(FontSans, position, size, "Playing...");
	else
		queueString(FontSans, position, size, "Paused.");
	position[1] -= 1.25f;
	queueString(FontMono, position, size, "%.4fx", replay->speed);
	// frame is a zero-based index, we show it one-based
	position[1] -= 2.0f;
	queueString(FontMono, position, size, "%d", (int)replay->frame + 1);
	position[1] -= 1.2f;
	queueString(FontMono, position, size, "%d", replay->frames->count);
}

void renderText(void)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(program);
	glBindVertexArray(vao);

	for (int i = 0; i < FontSize; i++) {
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER,
		             sizeof(struct textVertex) * VERTEX_LIMIT, NULL,
		             GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0,
		                (GLsizeiptr)min(textQueue[i]->count,
		                                VERTEX_LIMIT)
		                * sizeof(struct textVertex),
		                textQueue[i]->buffer);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fonts[i].atlas);

		glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
		glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
		glUniform4fv(colorAttr, 1, color);
		glUniform1f(pxRangeAttr, fonts[i].atlasRange);
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)textQueue[i]->count);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glBindVertexArray(0);
	glUseProgram(0);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	for (int i = 0; i < FontSize; i++)
		clearQueue(textQueue[i]);
}