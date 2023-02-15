#pragma once

#include <memory>
#include <mutex>
#include "SDL_events.h"
#include "vuk/Future.hpp"
#include "vuk/Image.hpp"
#include "util/math.hpp"
#include "gfx/resource.hpp"

namespace minote {

// Imgui wrapper to simplify the invocations
struct Imgui {
	
	struct InputReader {
		
		// Process the event by Imgui. Returns true if Imgui handled the event
		// and it shouldn't be processed by the application
		bool process(SDL_Event const&);
		
	};
	
	Imgui(vuk::Allocator&);
	~Imgui();
	
	// Compile required shaders; optional
	static void compile();
	
	// Obtain the input reader to process user input events
	auto getInputReader() -> InputReader;
	
	// After all user input is provided, begin the frame. ImGui:: calls are now allowed
	void begin();
	
	// After all controls are created, draw Imgui to the provided texture
	auto render(Texture2D<float4> target) -> Texture2D<float4>;
	
	Imgui(Imgui const&) = delete;
	auto operator=(Imgui const&) -> Imgui& = delete;
	
private:
	
	static inline bool m_exists = false;
	static inline bool m_compiled = false;
	std::recursive_mutex m_stateLock;
	bool m_insideFrame;
	
	vuk::Texture m_font;
	
	void setTheme();
	void uploadFont(vuk::Allocator&); // Initialize m_font
	
};

}
