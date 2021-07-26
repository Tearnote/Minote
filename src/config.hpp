#pragma once

#ifndef NDEBUG
	#define LOG_LEVEL quill::LogLevel::TraceL3
#else
	#define LOG_LEVEL quill::LogLevel::Info
#endif

#ifndef NDEBUG
	#define VK_VALIDATION 1
#else
	#define VK_VALIDATION 0
#endif

static constexpr auto AssetsPath = "assets.db";
