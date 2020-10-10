/**
 * GLAD wrapper that includes the appropriate header for the build mode.
 * @file
 */

#ifndef NDEBUG
#include "debug/glad.h"
#else //NDEBUG
#include "release/glad.h"
#endif //NDEBUG
