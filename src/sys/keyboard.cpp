#include "sys/keyboard.hpp"

#include <GLFW/glfw3.h>
#include "base/util.hpp"

namespace minote::sys {

using namespace base;
using namespace base::literals;

Scancode::Scancode(Keycode keycode): code{glfwGetKeyScancode(+keycode)} {}

}
