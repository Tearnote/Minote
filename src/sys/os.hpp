#pragma once

namespace minote {

// Ugly OS-specific code lives here
struct OS {
	
	// Ensure that console output is visible and bound to standard input and output
	static void initConsole();
	
};

}
