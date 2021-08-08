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

#ifndef NDEBUG
inline constexpr auto Log_p = "minote-debug.log";
#else //NDEBUG
inline constexpr auto Log_p = "minote.log";
#endif //NDEBUG

inline constexpr auto Assets_p = "assets.db";
