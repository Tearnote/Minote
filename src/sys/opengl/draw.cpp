#include "sys/opengl/draw.hpp"

#include "sys/opengl/state.hpp"

namespace minote {

void DrawParams::set() const
{
	using detail::state;

	if (blending) {
		state.setFeature(GL_BLEND, true);
		state.setBlendingMode({+blendingMode.src, +blendingMode.dst});
	} else {
		state.setFeature(GL_BLEND, false);
	}

	state.setFeature(GL_CULL_FACE, culling);

	if (depthTesting) {
		state.setFeature(GL_DEPTH_TEST, true);
		state.setDepthFunc(+depthFunc);
	} else {
		state.setFeature(GL_DEPTH_TEST, false);
	}

	if (scissorTesting) {
		state.setFeature(GL_SCISSOR_TEST, true);
		state.setScissorBox(scissorBox);
	} else {
		state.setFeature(GL_SCISSOR_TEST, false);
	}

	if (stencilTesting) {
		state.setFeature(GL_STENCIL_TEST, true);
		state.setStencilMode({
			.func = +stencilMode.func,
			.ref = stencilMode.ref,
			.sfail = +stencilMode.sfail,
			.dpfail = +stencilMode.dpfail,
			.dppass = +stencilMode.dppass
		});
	} else {
		state.setFeature(GL_STENCIL_TEST, false);
	}

	state.setViewport(viewport);

	state.setColorWrite(colorWrite);
}

}
