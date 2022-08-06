#pragma once

#include "vuk/Future.hpp"

namespace minote {

// Some weak typedefs to add semantics to gfx function params

template<typename T>
using Texture2D = vuk::Future;

template<typename T>
using Buffer = vuk::Future;

}
