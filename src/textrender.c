// Minote - textrender.c

#include "textrender.h"

#include <stdlib.h>
#include <string.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <unicode/ptypes.h>
#include <unicode/ustring.h>

#include "linmath/linmath.h"

#include "font.h"
#include "render.h"
#include "log.h"
#include "queue.h"
#include "util.h"

#define VERTEX_LIMIT 8192

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;

struct textVertex {
	GLfloat x, y, z;
	GLfloat tx, ty;
};

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

static void queueGlyph(enum fontType font, int codepoint, const vec3 position,
                       GLfloat size)
{
	if (codepoint >= fonts[font].glyphCount)
		return;

	vec3 adjusted = {};
	adjusted[0] = position[0];// - (GLfloat)fonts[font].glyphs[codepoint].xOffset / (float)fonts[font].size * size;
	adjusted[1] = position[1];// - (GLfloat)fonts[font].glyphs[codepoint].yOffset / (float)fonts[font].size * size;
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

void
queueString(enum fontType font, const char *string, vec3 position, float size)
{
	UErrorCode icuError = U_ZERO_ERROR;
	UChar ustring[256];
	u_strFromUTF8(ustring, 256, NULL, string, -1, &icuError);
	if (U_FAILURE(icuError)) {
		logError("Text encoding failure: %s", u_errorName(icuError));
	}

	float advance = 0;
	vec3 cursor = {};
	memcpy(cursor, position, sizeof(cursor));
	for (unsigned int i = 0; i < u_strlen(ustring); ++i) {
		int codepoint = ustring[i];
		queueGlyph(font, ustring[i], cursor, size);
		if (codepoint >= fonts[font].glyphCount)
			continue;
		cursor[0] += (float)fonts[font].glyphs[codepoint].advance / (float)fonts[font].size * size;
	}
}

void queuePlayfieldText(void)
{
	float size = 0.75f;
	vec3 position = { -23.0f, 25.0f, 0.0f };
	position[1] -= size;
	queueString(FontSans, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSans, "VAVAKV Żółćąłńśź", position, size);
	position[1] -= size;
	queueString(FontSerif, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSerif, "VAVAKV Żółćąłńśź", position, size);
	size += 0.25f;
	position[1] -= size;
	queueString(FontSans, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSans, "VAVAKV Żółćąłńśź", position, size);
	position[1] -= size;
	queueString(FontSerif, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSerif, "VAVAKV Żółćąłńśź", position, size);
	size += 0.25f;
	position[1] -= size;
	queueString(FontSans, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSans, "VAVAKV Żółćąłńśź", position, size);
	position[1] -= size;
	queueString(FontSerif, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSerif, "VAVAKV Żółćąłńśź", position, size);
	size += 0.25f;
	position[1] -= size;
	queueString(FontSans, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSans, "VAVAKV Żółćąłńśź", position, size);
	position[1] -= size;
	queueString(FontSerif, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSerif, "VAVAKV Żółćąłńśź", position, size);
	size += 0.25f;
	position[1] -= size;
	queueString(FontSans, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSans, "VAVAKV Żółćąłńśź", position, size);
	position[1] -= size;
	queueString(FontSerif, "affinity 100%", position, size);
	position[1] -= size;
	queueString(FontSerif, "VAVAKV Żółćąłńśź", position, size);
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
		                (GLsizeiptr)MIN(textQueue[i]->count,
		                                VERTEX_LIMIT)
		                * sizeof(struct textVertex),
		                textQueue[i]->buffer);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fonts[i].atlas);

		glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
		glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
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