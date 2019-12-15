// Minote - renderer.h

#ifndef MINOTE_RENDERER_H
#define MINOTE_RENDERER_H

#include "window.h"

class Renderer {
public:
	explicit Renderer(Window&);
	~Renderer();

	auto render() -> void;

private:
	Window& window;
};

#endif //MINOTE_RENDERER_H

