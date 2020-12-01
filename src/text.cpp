/**
 * Implementation of text.h
 * @file
 */

#include "text.hpp"

#include <stdarg.h>
#include <stdio.h>
#include "sys/opengl/vertexarray.hpp"
#include "sys/opengl/texture.hpp"
#include "sys/opengl/shader.hpp"
#include "sys/opengl/buffer.hpp"
#include "sys/opengl/draw.hpp"
#include "base/array.hpp"
#include "base/util.hpp"
#include "store/fonts.hpp"
#include "base/log.hpp"

using namespace minote;

/// Single glyph instance for the MSDF shader
struct MsdfGlyph {
	vec2 position; ///< Glyph offset in the string (lower left)
	vec2 size; ///< Size of the glyph
	vec4 texBounds; ///< AABB of the atlas UVs
	color4 color; ///< Glyph color
	int transformIndex; ///< Index of the string transform from the "transforms" buffer texture
};

static constexpr size_t MaxGlyphs{1024};
static constexpr size_t MaxStrings{64};

static VertexArray msdfVao = {};
static VertexBuffer<MsdfGlyph> msdfGlyphsVbo;
static vector<MsdfGlyph, MaxGlyphs> msdfGlyphs;
static BufferTexture<mat4> msdfTransformsTex = {};
static vector<mat4, MaxStrings> msdfTransforms;
static Font* msdfFont = nullptr;

static Draw<Shaders::Msdf> msdf = {
	.mode = DrawMode::TriangleStrip,
	.triangles = 2,
	.params = {
		.blending = true
	}
};

static bool initialized = false;

static void textQueueV(Font& font, float size, vec3 pos, vec3 dir, vec3 up,
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
		hb_shape(font.hbFont, text, nullptr, 0);

		// Construct the string transform
		vec3 eye = vec3(pos.x, pos.y, pos.z) - vec3(dir.x, dir.y, dir.z);
		mat4 lookat = lookAt(vec3(pos.x, pos.y, pos.z), eye, vec3(up.x, up.y, up.z));
		mat4 inverted = inverse(lookat);
		msdfTransforms.push_back(scale(inverted, {size, size, size}));

		// Iterate over glyphs
		unsigned glyphCount = 0;
		hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(text,
			&glyphCount);
		hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions(text,
			&glyphCount);
		vec2 cursor {0};
		for (size_t i = 0; i < glyphCount; i += 1) {
			auto& glyph = msdfGlyphs.emplace_back();

			// Calculate glyph information
			size_t id = glyphInfo[i].codepoint;
			Font::Glyph* atlasChar = &font.metrics[id];
			vec2 offset = {
				glyphPos[i].x_offset / 1024.0f,
				glyphPos[i].y_offset / 1024.0f
			};
			float xAdvance = glyphPos[i].x_advance / 1024.0f;
			float yAdvance = glyphPos[i].y_advance / 1024.0f;

			// Fill in draw data
			glyph.position = cursor + offset + atlasChar->glyph.pos;
			glyph.size = atlasChar->glyph.size;
			glyph.texBounds.x =
				atlasChar->msdf.pos.x / (float)font.atlas.size.x;
			glyph.texBounds.y =
				atlasChar->msdf.pos.y / (float)font.atlas.size.y;
			glyph.texBounds.z =
				atlasChar->msdf.size.x / (float)font.atlas.size.x;
			glyph.texBounds.w =
				atlasChar->msdf.size.y / (float)font.atlas.size.y;
			glyph.color = color;
			glyph.transformIndex = msdfTransforms.size() - 1;

			// Advance position
			cursor.x += xAdvance;
			cursor.y += yAdvance;
		}
	} while(false);

	hb_buffer_destroy(text);

	msdfFont = &font;
}

void textInit(void)
{
	if (initialized) return;

	msdfGlyphsVbo.create("msdfGlyphVbo", true);
	msdfVao.create("msdfVao");
	msdfTransformsTex.create("msdfTransformTex", true);

	msdfVao.setAttribute(0, msdfGlyphsVbo, &MsdfGlyph::position, true);
	msdfVao.setAttribute(1, msdfGlyphsVbo, &MsdfGlyph::size, true);
	msdfVao.setAttribute(2, msdfGlyphsVbo, &MsdfGlyph::texBounds, true);
	msdfVao.setAttribute(3, msdfGlyphsVbo, &MsdfGlyph::color, true);
	msdfVao.setAttribute(4, msdfGlyphsVbo, &MsdfGlyph::transformIndex, true);

	initialized = true;
}

void textCleanup(void)
{
	if (!initialized) return;

	msdfTransformsTex.destroy();
	msdfGlyphsVbo.destroy();
	msdfVao.destroy();

	initialized = false;
}

void textQueue(Font& font, float size, vec3 pos, color4 color, const char* fmt, ...)
{
	ASSERT(initialized);
	ASSERT(fmt);

	va_list args;
	va_start(args, fmt);
	textQueueV(font, size, pos, (vec3){1.0f, 0.0f, 0.0f},
		(vec3){0.0f, 1.0f, 0.0f}, color, fmt, args);
	va_end(args);
}

void textQueueDir(Font& font, float size, vec3 pos, vec3 dir, vec3 up,
	color4 color, const char* fmt, ...)
{
	ASSERT(initialized);
	ASSERT(fmt);

	va_list args;
	va_start(args, fmt);
	textQueueV(font, size, pos, dir, up, color, fmt, args);
	va_end(args);
}

void textDraw(Engine& engine)
{
	ASSERT(initialized);

	if (!msdfGlyphs.size()) return;

	msdfGlyphsVbo.upload(msdfGlyphs);
	msdfTransformsTex.upload(msdfTransforms);

	msdf.shader = &engine.shaders.msdf;
	msdf.vertexarray = &msdfVao;
	msdf.framebuffer = engine.frame.fb;
	msdf.instances = msdfGlyphs.size();
	msdf.shader->atlas = msdfFont->atlas;
	msdf.shader->transforms = msdfTransformsTex;
	msdf.shader->projection = engine.scene.projection;
	msdf.shader->view = engine.scene.view;
	msdf.draw();

	msdfGlyphs.clear();
	msdfTransforms.clear();
}
