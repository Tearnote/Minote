#include "gfx/effects/clear.hpp"

#include "gfx/util.hpp"

namespace minote::gfx {

void Clear::apply(Frame& _frame, Texture2D _target, vuk::ClearColor _color) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_target.name, "clear"),
		.resources = {
			_target.resource(vuk::eTransferClear) },
		.execute = [_target, _color](vuk::CommandBuffer& cmd) {
			
			cmd.clear_image(_target.name, _color);
			
	}});
	
}

}
