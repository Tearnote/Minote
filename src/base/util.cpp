/**
 * Implementation of functions required by util.hpp
 * @file
 */

#include "base/util.hpp"

#include <cstring>
#include "base/log.hpp"

namespace minote {

}

namespace ppk::assert {

// Implementation of the assert failure callback
void implementation::throwException(AssertionException const& e)
{
	minote::L.fail(R"(Assertion "%s" triggered on line %d in %s%s%s)",
		e.expression(), e.line(), e.file(), std::strlen(e.what())? ": " : "", e.what());
}

}
