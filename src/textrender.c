// Minote - textrender.c

#include "textrender.h"

#include <stdlib.h>
#include <string.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <unicode/ptypes.h>
#include <unicode/ustring.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "util.h"
#define STBI_ASSERT(x) assert(x)
#include "stb_image/stb_image.h"
#include "linmath/linmath.h"

#include "render.h"
#include "log.h"
#include "queue.h"

#define FONT_NAME "Merriweather-Regular"
#include "Merriweather-Regular_desc.c"
#define FONT_PATH "ttf/"FONT_NAME".ttf"
#define FONT_IMG_PATH "ttf/"FONT_NAME"_img.png"

FT_Library freetype = NULL;
FT_Face face = NULL;
hb_font_t *fontShape = NULL;

#define VERTEX_LIMIT 8192

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vertexBuffer = 0;
static GLuint atlas = 0;
static int atlasSize = 0;

static GLint cameraAttr = -1;
static GLint projectionAttr = -1;

struct textVertex {
	GLfloat x, y, z;
	GLfloat tx, ty;
};
static queue *textQueue = NULL;

void initTextRenderer(void)
{
	FT_Error freetypeError = FT_Init_FreeType(&freetype);
	if (freetypeError) {
		logCrit("Failed to initialize text renderer: %s", FT_Error_String(freetypeError));
		exit(1);
	}
	freetypeError = FT_New_Face(freetype, FONT_PATH, 0, &face);
	if (freetypeError) {
		logCrit("Failed to load font %s: %s", FONT_PATH, FT_Error_String(freetypeError));
		exit(1);
	}
	FT_Set_Char_Size(face, 0, 0, 0, 0);
	fontShape = hb_ft_font_create(face, NULL);

	textQueue = createQueue(sizeof(struct textVertex));

	unsigned char *atlasData = NULL;
	int width;
	int height;
	int channels;
	atlasData = stbi_load(FONT_IMG_PATH, &width, &height, &channels, 4);
	if (!atlasData) {
		logCrit("Failed to load %s: %s", FONT_IMG_PATH,
		         stbi_failure_reason());
		exit(1);
	}
	assert(width == height);
	atlasSize = width;

	glGenTextures(1, &atlas);
	glBindTexture(GL_TEXTURE_2D, atlas);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, atlasData);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(atlasData);

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
	glDeleteTextures(1, &atlas);
	atlas = 0;

	hb_font_destroy(fontShape);
	FT_Done_Face(face);
	FT_Done_FreeType(freetype);
}

static void queueGlyph(hb_codepoint_t glyph, vec3 position, GLfloat size)
{
	if (glyph >= font_Merriweather_Regular_bitmap_chars_count)
		return;

	vec3 adjusted = {};
	adjusted[0] = position[0] + (GLfloat)font_Merriweather_Regular_codepoint_infos[glyph].minx / font_Merriweather_Regular_information.max_height * size;
	adjusted[1] = position[1] + (GLfloat)font_Merriweather_Regular_codepoint_infos[glyph].miny / font_Merriweather_Regular_information.max_height * size;
	adjusted[2] = position[2];
	
	vec3 bottomLeft = {};
	vec3 bottomRight = {};
	vec3 topLeft = {};
	vec3 topRight = {};

	GLfloat tx1 =
		(GLfloat)font_Merriweather_Regular_codepoint_infos[glyph].atlas_x;
	GLfloat ty1 =
		(GLfloat)font_Merriweather_Regular_codepoint_infos[glyph].atlas_y;
	GLfloat tx2 = tx1 + (GLfloat)font_Merriweather_Regular_codepoint_infos[glyph]
		.atlas_w;
	GLfloat ty2 = ty1 + (GLfloat)font_Merriweather_Regular_codepoint_infos[glyph]
		.atlas_h;
	GLfloat w = (tx2 - tx1)
	            / (GLfloat)font_Merriweather_Regular_information.max_height
	            * size;
	GLfloat h = (ty2 - ty1)
	            / (GLfloat)font_Merriweather_Regular_information.max_height
	            * size;
	tx1 /= (GLfloat)atlasSize;
	ty1 /= (GLfloat)atlasSize;
	tx2 /= (GLfloat)atlasSize;
	ty2 /= (GLfloat)atlasSize;
	ty1 = 1.0f - ty1;
	ty2 = 1.0f - ty2;

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
	newVertex = produceQueueItem(textQueue);
	newVertex->x = bottomLeft[0];
	newVertex->y = bottomLeft[1];
	newVertex->z = bottomLeft[2];
	newVertex->tx = tx1;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = bottomRight[0];
	newVertex->y = bottomRight[1];
	newVertex->z = bottomRight[2];
	newVertex->tx = tx2;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = topRight[0];
	newVertex->y = topRight[1];
	newVertex->z = topRight[2];
	newVertex->tx = tx2;
	newVertex->ty = ty2;

	newVertex = produceQueueItem(textQueue);
	newVertex->x = bottomLeft[0];
	newVertex->y = bottomLeft[1];
	newVertex->z = bottomLeft[2];
	newVertex->tx = tx1;
	newVertex->ty = ty1;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = topRight[0];
	newVertex->y = topRight[1];
	newVertex->z = topRight[2];
	newVertex->tx = tx2;
	newVertex->ty = ty2;
	newVertex = produceQueueItem(textQueue);
	newVertex->x = topLeft[0];
	newVertex->y = topLeft[1];
	newVertex->z = topLeft[2];
	newVertex->tx = tx1;
	newVertex->ty = ty2;
}

void queueString(const char *string, vec3 position, float size)
{
	UErrorCode icuError = U_ZERO_ERROR;
	UChar ustring[256];
	u_strFromUTF8(ustring, 256, NULL, string, -1, &icuError);
	if (U_FAILURE(icuError)) {
		logError("Text encoding failure: %s", u_errorName(icuError));
	}

	hb_buffer_t *buf;
	buf = hb_buffer_create();
	hb_buffer_add_utf16(buf, ustring, u_strlen(ustring), 0, -1);
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
	hb_buffer_set_language(buf, hb_language_from_string("en", -1));
	hb_shape(fontShape, buf, NULL, 0);
	unsigned int glyphCount = 0;
	hb_glyph_info_t *glyphInfo = hb_buffer_get_glyph_infos(buf, &glyphCount);
	hb_glyph_position_t *glyphPos = hb_buffer_get_glyph_positions(buf, &glyphCount);

	float xOffset = 0;
	float yOffset = 0;
	float xAdvance = 0;
	float yAdvance = 0;
	logDebug("Start");
	for (unsigned int i = 0; i < glyphCount; ++i) {
		hb_codepoint_t codepoint = glyphInfo[i].codepoint;
		logDebug("Index: %u", codepoint);
		xOffset = glyphPos[i].x_offset / 85.0f * size;
		yOffset = glyphPos[i].y_offset / 85.0f * size;
		xAdvance = glyphPos[i].x_advance / 85.0f * size;
		yAdvance = glyphPos[i].y_advance / 85.0f * size;
		vec3 adjusted = {};
		adjusted[0] = position[0] + xOffset;
		adjusted[1] = position[1] + yOffset;
		adjusted[2] = position[2];
		queueGlyph(codepoint, position, size);
		position[0] += xAdvance;
		position[1] += yAdvance;
	}

	hb_buffer_destroy(buf);
}

void queuePlayfieldText(void)
{
	vec3 position = { -17.0f, 0.0f, 1.0f };
	queueString("affinity 100% VAVAKV Żółćąłńśź", position, 8.0f);
}

void renderText(void)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct textVertex) * VERTEX_LIMIT,
	             NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
	                (GLsizeiptr)MIN(textQueue->count, VERTEX_LIMIT)
	                * sizeof(struct textVertex), textQueue->buffer);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(program);
	glBindVertexArray(vao);
	glBindTexture(GL_TEXTURE_2D, atlas);

	glUniformMatrix4fv(cameraAttr, 1, GL_FALSE, camera[0]);
	glUniformMatrix4fv(projectionAttr, 1, GL_FALSE, projection[0]);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)textQueue->count);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	clearQueue(textQueue);
}