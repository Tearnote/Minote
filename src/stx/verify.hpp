#pragma once

#include "config.hpp"
#include "assert.hpp"

// Provides assertion macros:
// DEBUG_ASSERT() - checks condition in debug, no codegen in release
// ASSERT() - checks condition in debug, expression pass-through in release
// ASSUME() - checks condition in debug, marks failure as unreachable in release
// VERIFY() - checks condition in all modes. Generally should use exceptions instead
