/**
 * Implementation of text.h
 * @file
 */

#include "text.hpp"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include "linmath/linmath.h"
#include "sys/opengl.hpp"
#include "base/varray.hpp"
#include "world.hpp"
#include "base/util.hpp"
#include "font.hpp"
#include "base/log.hpp"

using namespace minote;

/// Shader type for MSDF drawing
typedef struct ProgramMsdf {
	ProgramBase base;
	TextureUnit transforms; ///< Buffer texture containing per-string transforms
	TextureUnit atlas; ///< Font atlas
	Uniform camera;
	Uniform projection;
} ProgramMsdf;

/// Single glyph instance for the MSDF shader
typedef struct GlyphMsdf {
	point2f position; ///< Glyph offset in the string (lower left)
	size2f size; ///< Size of the glyph
	point4f texBounds; ///< AABB of the atlas UVs
	color4 color; ///< Glyph color
	int transformIndex; ///< Index of the string transform from the "transforms" buffer texture
} GlyphMsdf;

static const char* ProgramMsdfVertName = "msdf.vert";
static const GLchar* ProgramMsdfVertSrc = (GLchar[]){
#include "msdf.vert"
	'\0'};
static const char* ProgramMsdfFragName = "msdf.frag";
static const GLchar* ProgramMsdfFragSrc = (GLchar[]){
#include "msdf.frag"
	'\0'};

static constexpr std::size_t MaxGlyphs{1024};
static constexpr std::size_t MaxStrings{1024};

static ProgramMsdf* msdf = nullptr;
static VertexArray msdfVao[FontSize] = {0};
static VertexBuffer msdfGlyphsVbo[FontSize] = {0};
static varray<GlyphMsdf, MaxGlyphs> msdfGlyphs[FontSize]{};
static BufferTextureStorage msdfTransformsStorage[FontSize] = {0};
static BufferTexture msdfTransformsTex[FontSize] = {0};
static varray<mat4x4, MaxStrings> msdfTransforms[FontSize]{};

static bool initialized = false;

static void textQueueV(FontType font, float size, point3f pos, point3f dir, point3f up,
	color4 color, const char* fmt, va_list args)
{
	hb_buffer_t* text{nullptr};
	do {
		// Costruct the formatted string
		va_list argsCopy;
		va_copy(argsCopy, args);
		size_t stringSize = vsnprintf(nullptr, 0, fmt, args) + 1;
		char string[stringSize];
		vsnprintf(string, stringSize, fmt, argsCopy);

		// Pass string to HarfBuzz and shape it
		text = hb_buffer_create();
		hb_buffer_add_utf8(text, string, -1, 0, -1);
		hb_buffer_set_direction(text, HB_DIRECTION_LTR);
		hb_buffer_set_script(text, HB_SCRIPT_LATIN);
		hb_buffer_set_language(text, hb_language_from_string("en", -1));
		hb_shape(fonts[font].hbFont, text, nullptr, 0);

		// Construct the string transform
		auto* const transform = msdfTransforms[font].produce();
		if (!transform)
			break;
		vec3 eye = {0};
		vec3_sub(eye, pos.arr().data(), dir.arr().data());
		mat4x4 lookat = {0};
		mat4x4 inverted = {0};
		mat4x4_look_at(lookat, pos.arr().data(), eye, up.arr().data());
		mat4x4_invert(inverted, lookat);
		mat4x4_scale_aniso(*transform, inverted, size, size, size);

		// Iterate over glyphs
		unsigned glyphCount = 0;
		hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(text,
			&glyphCount);
		hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions(text,
			&glyphCount);
		point2f cursor = {0};
		for (size_t i = 0; i < glyphCount; i += 1) {
			auto* const glyph = msdfGlyphs[font].produce();
			if (!glyph)
				break;

			// Calculate glyph information
			size_t id = glyphInfo[i].codepoint;
			FontAtlasGlyph* atlasChar = &fonts[font].metrics[id];
			float xOffset = glyphPos[i].x_offset / 1024.0f;
			float yOffset = glyphPos[i].y_offset / 1024.0f;
			float xAdvance = glyphPos[i].x_advance / 1024.0f;
			float yAdvance = glyphPos[i].y_advance / 1024.0f;

			// Fill in draw data
			glyph->position.x = cursor.x + xOffset + atlasChar->charLeft;
			glyph->position.y = cursor.y + yOffset + atlasChar->charBottom;
			glyph->size.x = atlasChar->charRight - atlasChar->charLeft;
			glyph->size.y = atlasChar->charTop - atlasChar->charBottom;
			glyph->texBounds.x =
				atlasChar->atlasLeft / (float)fonts[font].atlas.size.x;
			glyph->texBounds.y =
				atlasChar->atlasBottom / (float)fonts[font].atlas.size.y;
			glyph->texBounds.z =
				atlasChar->atlasRight / (float)fonts[font].atlas.size.x;
			glyph->texBounds.w =
				atlasChar->atlasTop / (float)fonts[font].atlas.size.y;
			glyph->color = color;
			glyph->transformIndex = msdfTransforms[font].size - 1;

			// Advance position
			cursor.x += xAdvance;
			cursor.y += yAdvance;
		}
	} while(false);

	hb_buffer_destroy(text);
}

void textInit(void)
{
	if (initialized) return;

	msdf = programCreate(ProgramMsdf,
		ProgramMsdfVertName, ProgramMsdfVertSrc,
		ProgramMsdfFragName, ProgramMsdfFragSrc);
	msdf->atlas = programSampler(msdf, "atlas", GL_TEXTURE0);
	msdf->transforms = programSampler(msdf, "transforms", GL_TEXTURE1);
	msdf->projection = programUniform(msdf, "projection");
	msdf->camera = programUniform(msdf, "camera");

	glGenBuffers(FontSize, msdfGlyphsVbo);
	glGenVertexArrays(FontSize, msdfVao);
	glGenBuffers(FontSize, msdfTransformsStorage);
	glGenTextures(FontSize, msdfTransformsTex);

	for (size_t i = 0; i < FontSize; i += 1) {
		glBindVertexArray(msdfVao[i]);
		glBindBuffer(GL_ARRAY_BUFFER, msdfGlyphsVbo[i]);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphMsdf),
			(void*)0);
		glVertexAttribDivisor(0, 1);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphMsdf),
			(void*)offsetof(GlyphMsdf, size));
		glVertexAttribDivisor(1, 1);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphMsdf),
			(void*)offsetof(GlyphMsdf, texBounds));
		glVertexAttribDivisor(2, 1);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphMsdf),
			(void*)offsetof(GlyphMsdf, color));
		glVertexAttribDivisor(3, 1);

		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 1, GL_INT, sizeof(GlyphMsdf),
			(void*)offsetof(GlyphMsdf, transformIndex));
		glVertexAttribDivisor(4, 1);

		glBindBuffer(GL_TEXTURE_BUFFER, msdfTransformsStorage[i]);
		glBufferData(GL_TEXTURE_BUFFER, 0, nullptr, GL_STREAM_DRAW);
		glBindTexture(GL_TEXTURE_BUFFER, msdfTransformsTex[i]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, msdfTransformsStorage[i]);

		L.debug("Initialized font %s", FontList[i]);
	}

	initialized = true;
}

void textCleanup(void)
{
	if (!initialized) return;

	glDeleteTextures(FontSize, msdfTransformsTex);
	glDeleteBuffers(FontSize, msdfTransformsStorage);
	glDeleteBuffers(FontSize, msdfGlyphsVbo);
	arrayClear(msdfGlyphsVbo);
	glDeleteVertexArrays(FontSize, msdfVao);
	arrayClear(msdfVao);

	programDestroy(msdf);
	msdf = nullptr;

	L.debug("Fonts cleaned up");
	initialized = false;
}

void textQueue(FontType font, float size, point3f pos, color4 color, const char* fmt, ...)
{
	ASSERT(initialized);
	ASSERT(fmt);

	va_list args;
	va_start(args, fmt);
	textQueueV(font, size, pos, (point3f){1.0f, 0.0f, 0.0f},
		(point3f){0.0f, 1.0f, 0.0f}, color, fmt, args);
	va_end(args);
}

void textQueueDir(FontType font, float size, point3f pos, point3f dir, point3f up,
	color4 color, const char* fmt, ...)
{
	ASSERT(initialized);
	ASSERT(fmt);

	va_list args;
	va_start(args, fmt);
	textQueueV(font, size, pos, dir, up, color, fmt, args);
	va_end(args);
}

void textDraw(void)
{
	ASSERT(initialized);

	for (size_t i = 0; i < FontSize; i += 1) {
		if (!msdfGlyphs[i].size) continue;

		size_t instances = msdfGlyphs[i].size;

		size_t glyphsSize = sizeof(GlyphMsdf) * msdfGlyphs[i].size;
		glBindBuffer(GL_ARRAY_BUFFER, msdfGlyphsVbo[i]);
		glBufferData(GL_ARRAY_BUFFER, glyphsSize, nullptr, GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, glyphsSize, msdfGlyphs[i].data());

		size_t transformsSize = sizeof(mat4x4) * msdfTransforms[i].size;
		glBindBuffer(GL_TEXTURE_BUFFER, msdfTransformsStorage[i]);
		glBufferData(GL_TEXTURE_BUFFER, transformsSize, nullptr, GL_STREAM_DRAW);
		glBufferSubData(GL_TEXTURE_BUFFER, 0, transformsSize, msdfTransforms[i].data());

		glBindVertexArray(msdfVao[i]);
		programUse(msdf);
		fonts[i].atlas.bind(msdf->atlas);
		glActiveTexture(msdf->transforms);
		glBindTexture(GL_TEXTURE_BUFFER, msdfTransformsTex[i]);
		glUniformMatrix4fv(msdf->projection, 1, GL_FALSE, *worldProjection);
		glUniformMatrix4fv(msdf->camera, 1, GL_FALSE, *worldCamera);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instances);

		msdfGlyphs[i].clear();
		msdfTransforms[i].clear();
	}
}
