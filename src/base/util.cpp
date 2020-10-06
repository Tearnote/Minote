/**
 * Implementation of functions required by util.hpp
 * @file
 */

#include "base/util.hpp"

#include <cstring>
#include "base/log.hpp"

using namespace minote;

namespace ppk::assert {

void implementation::throwException(const AssertionException& e)
{
	L.fail(R"(Assertion "%s" triggered on line %d in %s%s%s)",
		e.expression(), e.line(), e.file(), std::strlen(e.what())? ": " : "", e.what());
}

}
