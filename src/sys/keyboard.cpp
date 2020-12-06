#include "sys/keyboard.hpp"

#include <GLFW/glfw3.h>
#include "base/util.hpp"

namespace minote {

Scancode::Scancode(Keycode keycode): code{glfwGetKeyScancode(+keycode)} {}

}
