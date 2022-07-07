#pragma once

// Make sure BUILD_TYPE is specified

#define BUILD_DEBUG 0
#define BUILD_RELDEB 1
#define BUILD_RELEASE 2

#ifndef BUILD_TYPE
#error Build type not defined
#endif

#if (BUILD_TYPE != BUILD_DEBUG) && (BUILD_TYPE != BUILD_RELDEB) && (BUILD_TYPE != BUILD_RELEASE)
#error Build type incorrectly defined
#endif

// Build configuration variables

// Level of logging to file and/or console
#if BUILD_TYPE != BUILD_RELEASE
#define LOG_LEVEL quill::LogLevel::TraceL3
#else
#define LOG_LEVEL quill::LogLevel::Info
#endif

// Whether to spawn a console window for logging
#if BUILD_TYPE != BUILD_RELEASE
#define SPAWN_CONSOLE 1
#else
#define SPAWN_CONSOLE 0
#endif

// Whether Vulkan validation layers are enabled
#if BUILD_TYPE != BUILD_RELEASE
#define VK_VALIDATION 1
#else
#define VK_VALIDATION 0
#endif

// Logfile location
#if BUILD_TYPE == BUILD_DEBUG
inline constexpr auto Log_p = "minote-debug.log";
#elif BUILD_TYPE == BUILD_RELDEB
inline constexpr auto Log_p = "minote-reldeb.log";
#else
inline constexpr auto Log_p = "minote.log";
#endif

// Assets location
inline constexpr auto Assets_p = ASSETS_P;
