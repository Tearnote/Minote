// Minote - textrender.c

#include "textrender.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "render.h"
#include "util.h"
#include "log.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ASSERT(x) assert(x)
#include "stb_image/stb_image.h"

#define FONT_PATH "ttf/Bitter-Regular_img.png"
#include "Bitter-Regular_desc.c"

GLuint atlas = 0;

void initTextRenderer(void)
{
	unsigned char *atlasData = NULL;
	int width;
	int height;
	int channels;
	atlasData = stbi_load(FONT_PATH, &width, &height, &channels, 4);
	if (!atlasData) {
		logError("Failed to load %s: %s", FONT_PATH, stbi_failure_reason());
		return;
	}
	assert(width == 512 && height == 512);

	glGenTextures(1, &atlas);
	glBindTexture(GL_TEXTURE_2D, atlas);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlasData);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(atlasData);
}

void cleanupTextRenderer(void)
{
	glDeleteTextures(1, &atlas);
	atlas = 0;
}

void renderText(void)
{
	;
}