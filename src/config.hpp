#pragma once

#ifndef NDEBUG
#define IMGUI 1
#else //IMGUI
#define IMGUI 1
#endif //IMGUI

#ifndef NDEBUG
#define VK_VALIDATION 1
#else //IMGUI
#define VK_VALIDATION 0
#endif //IMGUI

static constexpr auto AssetsPath = "assets.db";
