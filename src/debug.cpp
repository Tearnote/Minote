/**
 * Implementation of debug.h
 * @file
 */

#define MINOTE_NO_NUKLEAR_INCLUDE
#include "debug.hpp"

#include <atomic>
#include <GLFW/glfw3.h>
#define NK_IMPLEMENTATION
#include "nuklear/nuklear.h"
#include "sys/opengl/vertexarray.hpp"
#include "sys/opengl/texture.hpp"
#include "sys/opengl/buffer.hpp"
#include "sys/opengl/shader.hpp"
#include "sys/opengl/draw.hpp"
#include "base/util.hpp"
#include "base/log.hpp"

using namespace minote;

#define NUKLEAR_VBO_SIZE (1024 * 1024)
#define NUKLEAR_EBO_SIZE (256 * 1024)

struct NuklearVertex {
	vec2 pos;
	vec2 texCoord;
	color4 color;
};

static const struct nk_draw_vertex_layout_element nuklearVertexLayout[] = {
	{NK_VERTEX_POSITION, NK_FORMAT_FLOAT,    offsetof(NuklearVertex, pos)},
	{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT,    offsetof(NuklearVertex, texCoord)},
	{NK_VERTEX_COLOR,    NK_FORMAT_R8G8B8A8, offsetof(NuklearVertex, color)},
	{NK_VERTEX_LAYOUT_END}
};

static struct nk_context nkContext = {0};

static struct nk_font_atlas atlas = {};
static struct nk_draw_null_texture nullTexture = {};
static struct nk_buffer commandList = {0};

static Texture<PixelFmt::RGBA_u8> nuklearTexture;

static VertexArray nuklearVao;
static VertexBuffer<NuklearVertex> nuklearVbo;
static ElementBuffer nuklearEbo;

static Draw<Shaders::Nuklear> nuklear = {
	.vertexarray = &nuklearVao,
	.params = {
		.blending = true,
		.culling = false,
		.depthTesting = false,
		.scissorTesting = true
	}
};

static std::atomic<ivec2> cursorPos;
static std::atomic<bool> leftClick;
static std::atomic<bool> rightClick;

static bool initialized = false;
static bool nuklearEnabled = true;

static void debugCursorPosCallback(GLFWwindow* w, double x, double y)
{
	(void)w;

	ivec2 newPos = {static_cast<int>(x), static_cast<int>(y)};
	atomic_store(&cursorPos, newPos);
}

static void debugMouseButtonCallback(GLFWwindow* w, int button, int action, int mods)
{
	(void)w, (void)mods;

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS)
			atomic_store(&leftClick, true);
		else
			atomic_store(&leftClick, false);
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS)
			atomic_store(&rightClick, true);
		else
			atomic_store(&rightClick, false);
	}
}

void debugInputSetup(Window& window)
{
	atomic_init(&cursorPos, ((ivec2){-1, -1}));
	atomic_init(&leftClick, false);
	atomic_init(&rightClick, false);
//	glfwSetCursorPosCallback(window.handle, debugCursorPosCallback);
//	glfwSetMouseButtonCallback(window.handle, debugMouseButtonCallback);
}

void debugInit(void)
{
	if (initialized) return;

	// Initialize font baker
	nk_font_atlas_init_default(&atlas);
	nk_font_atlas_begin(&atlas);

	// Bake the font atlas (default font included implicitly)
	const void* atlasData;
	int atlasWidth, atlasHeight;
	atlasData = nk_font_atlas_bake(&atlas, &atlasWidth, &atlasHeight,
		NK_FONT_ATLAS_RGBA32);

	// Upload font atlas to GPU
	nuklearTexture.create("nuklearTexture", {atlasWidth, atlasHeight});
	nuklearTexture.upload(span{reinterpret_cast<u8 const*>(atlasData),
		static_cast<size_t>(atlasWidth * atlasHeight * 4)}, 4);

	nk_font_atlas_end(&atlas, nk_handle_ptr(&nuklearTexture), &nullTexture);
	nk_init_default(&nkContext, &atlas.default_font->handle);

	// Set the theme
	struct nk_color table[NK_COLOR_COUNT];
	table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
	table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
	table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
	table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
	table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
	table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
	table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
	table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
	table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
	table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
	table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
	table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
	table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
	table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
	table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
	table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
	table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
	table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
	table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
	nk_style_from_table(&nkContext, table);

	// Set up the buffers
	nuklearVbo.create("nuklearVbo", true);
	nuklearEbo.create("nuklearEbo", true);
	nuklearVao.create("nuklearVao");
	nuklearVao.setAttribute(0, nuklearVbo, &NuklearVertex::pos);
	nuklearVao.setAttribute(1, nuklearVbo, &NuklearVertex::texCoord);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
		sizeof(NuklearVertex), (void*)offsetof(NuklearVertex, color));
	nuklearVao.setElements(nuklearEbo);
	nk_buffer_init_default(&commandList);

	nuklearEnabled = true;
	initialized = true;
	L.debug("Debug layer initialized");
}

void debugCleanup(void)
{
	if (!initialized) return;

	nk_buffer_free(&commandList);
	nuklearEbo.destroy();
	nuklearVbo.destroy();
	nuklearVao.destroy();
	nuklearTexture.destroy();
	nk_font_atlas_cleanup(&atlas);
	nk_free(&nkContext);

	initialized = false;
	L.debug("Debug layer cleaned up");
}

void debugUpdate(void)
{
	DASSERT(initialized);

	if (atomic_load(&rightClick)) {
		atomic_store(&rightClick, false);
		nuklearEnabled = !nuklearEnabled;
	}
	if (!nuklearEnabled) return;

	nk_input_begin(&nkContext);

	ivec2 newCursorPos = atomic_load(&cursorPos);
	nk_input_motion(&nkContext, newCursorPos.x, newCursorPos.y);
	nk_input_button(&nkContext, NK_BUTTON_LEFT, newCursorPos.x, newCursorPos.y,
		atomic_load(&leftClick));

	nk_input_end(&nkContext);
}

void debugDraw(Engine& engine)
{
	DASSERT(initialized);
	if (!nuklearEnabled) {
		nk_clear(&nkContext);
		return;
	}

	nuklear.shader = &engine.shaders.nuklear;
	nuklear.shader->atlas = nuklearTexture;
	nuklear.shader->projection = engine.scene.projection2D;
	nuklear.framebuffer = engine.frame.fb;

	nuklearVao.bind();
	nuklearVbo.bind();
	nuklearEbo.bind();
	glBufferData(GL_ARRAY_BUFFER, NUKLEAR_VBO_SIZE, nullptr, GL_STREAM_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, NUKLEAR_EBO_SIZE, nullptr,
		GL_STREAM_DRAW);

	// Convert the Nuklear command list into vertex inputs
	void* vboData = nullptr;
	void* eboData = nullptr;
	vboData = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	eboData = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	struct nk_convert_config convertConfig = {
		.global_alpha = 1.0f,
		.line_AA = NK_ANTI_ALIASING_ON,
		.shape_AA = NK_ANTI_ALIASING_ON,
		.circle_segment_count = 22,
		.arc_segment_count = 22,
		.curve_segment_count = 22,
		.null = nullTexture,
		.vertex_layout = nuklearVertexLayout,
		.vertex_size = sizeof(NuklearVertex),
		.vertex_alignment = alignof(NuklearVertex)
	};
	struct nk_buffer vboBuffer = {0};
	struct nk_buffer eboBuffer = {0};
	nk_buffer_init_fixed(&vboBuffer, vboData, NUKLEAR_VBO_SIZE);
	nk_buffer_init_fixed(&eboBuffer, eboData, NUKLEAR_EBO_SIZE);
	nk_convert(&nkContext, &commandList, &vboBuffer, &eboBuffer, &convertConfig);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	// Execute draw commands
	uvec2 screenSize = engine.window.size();
	const struct nk_draw_command* command;
	int offset = 0;
	nk_draw_foreach(command, &nkContext, &commandList) {
		if (!command->elem_count) continue;
		nuklear.shader->atlas = *static_cast<Texture<PixelFmt::RGBA_u8>*>(command->texture.ptr);
		nuklear.triangles = command->elem_count / 3;
		nuklear.offset = offset;
		nuklear.params.scissorBox = {
			{command->clip_rect.x, screenSize.y - command->clip_rect.y - command->clip_rect.h},
			{command->clip_rect.w, command->clip_rect.h}
		};
		nuklear.draw();
		offset += command->elem_count;
	}
	nk_clear(&nkContext);
	nk_buffer_clear(&commandList);
}

struct nk_context* nkCtx(void)
{
	return &nkContext;
}
