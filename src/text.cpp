/**
 * Implementation of text.h
 * @file
 */

#include "text.hpp"

#include <stdarg.h>
#include <stdio.h>
#include "sys/opengl.hpp"
#include "base/varray.hpp"
#include "world.hpp"
#include "base/util.hpp"
#include "font.hpp"
#include "base/log.hpp"
#include "renderer.hpp"

using namespace minote;

/// Shader type for MSDF drawing
struct MsdfShader : Shader {

	BufferSampler transforms; ///< Buffer texture containing per-string transforms
	Sampler<Texture> atlas; ///< Font atlas
	Uniform<mat4> camera;
	Uniform<mat4> projection;

	void setLocations() override
	{
		atlas.setLocation(*this, "atlas", TextureUnit::_0);
		transforms.setLocation(*this, "transforms", TextureUnit::_1);
		projection.setLocation(*this, "projection");
		camera.setLocation(*this, "camera");
	}

};

/// Single glyph instance for the MSDF shader
struct MsdfGlyph {
	vec2 position; ///< Glyph offset in the string (lower left)
	vec2 size; ///< Size of the glyph
	vec4 texBounds; ///< AABB of the atlas UVs
	color4 color; ///< Glyph color
	int transformIndex; ///< Index of the string transform from the "transforms" buffer texture
};

static const GLchar* MsdfVertSrc = (GLchar[]){
#include "msdf.vert"
	'\0'};
static const GLchar* MsdfFragSrc = (GLchar[]){
#include "msdf.frag"
	'\0'};

static constexpr std::size_t MaxGlyphs{1024};
static constexpr std::size_t MaxStrings{1024};

static MsdfShader msdfShader;
static VertexArray msdfVao[FontSize] = {};
static VertexBuffer<MsdfGlyph> msdfGlyphsVbo[FontSize];
static varray<MsdfGlyph, MaxGlyphs> msdfGlyphs[FontSize]{};
static BufferTexture<mat4> msdfTransformsTex[FontSize] = {};
static varray<mat4, MaxStrings> msdfTransforms[FontSize]{};

static Draw<MsdfShader> msdf = {
	.shader = &msdfShader,
	.framebuffer = &renderFb,
	.mode = DrawMode::TriangleStrip,
	.triangles = 2,
	.params = {
		.blending = true
	}
};

static bool initialized = false;

static void textQueueV(FontType font, float size, vec3 pos, vec3 dir, vec3 up,
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
		vec3 eye = vec3(pos.x, pos.y, pos.z) - vec3(dir.x, dir.y, dir.z);
		mat4 lookat = lookAt(vec3(pos.x, pos.y, pos.z), eye, vec3(up.x, up.y, up.z));
		mat4 inverted = inverse(lookat);
		*transform = scale(inverted, {size, size, size});

		// Iterate over glyphs
		unsigned glyphCount = 0;
		hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(text,
			&glyphCount);
		hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions(text,
			&glyphCount);
		vec2 cursor {0};
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

	msdfShader.create("msdf", MsdfVertSrc, MsdfFragSrc);

	for (auto& msdfGlyphVbo : msdfGlyphsVbo)
		msdfGlyphVbo.create("msdfGlyphVbo", true);
	for (auto& vao : msdfVao)
		vao.create("msdfVao");
	for (auto& texture : msdfTransformsTex)
		texture.create("msdfTransformTex", true);

	for (size_t i = 0; i < FontSize; i += 1) {
		msdfVao[i].setAttribute(0, msdfGlyphsVbo[i], &MsdfGlyph::position, true);
		msdfVao[i].setAttribute(1, msdfGlyphsVbo[i], &MsdfGlyph::size, true);
		msdfVao[i].setAttribute(2, msdfGlyphsVbo[i], &MsdfGlyph::texBounds, true);
		msdfVao[i].setAttribute(3, msdfGlyphsVbo[i], &MsdfGlyph::color, true);
		msdfVao[i].setAttribute(4, msdfGlyphsVbo[i], &MsdfGlyph::transformIndex, true);

		L.debug("Initialized font %s", FontList[i]);
	}

	initialized = true;
}

void textCleanup(void)
{
	if (!initialized) return;

	for (auto& texture : msdfTransformsTex)
		texture.destroy();
	for (auto& msdfGlyphVbo : msdfGlyphsVbo)
		msdfGlyphVbo.destroy();
	for (auto& vao : msdfVao)
		vao.destroy();

	msdfShader.destroy();

	L.debug("Fonts cleaned up");
	initialized = false;
}

void textQueue(FontType font, float size, vec3 pos, color4 color, const char* fmt, ...)
{
	ASSERT(initialized);
	ASSERT(fmt);

	va_list args;
	va_start(args, fmt);
	textQueueV(font, size, pos, (vec3){1.0f, 0.0f, 0.0f},
		(vec3){0.0f, 1.0f, 0.0f}, color, fmt, args);
	va_end(args);
}

void textQueueDir(FontType font, float size, vec3 pos, vec3 dir, vec3 up,
	color4 color, const char* fmt, ...)
{
	ASSERT(initialized);
	ASSERT(fmt);

	va_list args;
	va_start(args, fmt);
	textQueueV(font, size, pos, dir, up, color, fmt, args);
	va_end(args);
}

void textDraw(Window& window)
{
	ASSERT(initialized);

	for (size_t i = 0; i < FontSize; i += 1) {
		if (!msdfGlyphs[i].size) continue;

		msdfGlyphsVbo[i].upload(msdfGlyphs[i]);
		msdfTransformsTex[i].upload(msdfTransforms[i]);

		msdf.vertexarray = &msdfVao[i];
		msdf.instances = msdfGlyphs[i].size;
		msdf.shader->atlas = fonts[i].atlas;
		msdf.shader->transforms = msdfTransformsTex[i];
		msdf.shader->projection = worldProjection;
		msdf.shader->camera = worldCamera;
		msdf.draw(window);

		msdfGlyphs[i].clear();
		msdfTransforms[i].clear();
	}
}
