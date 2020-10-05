/**
 * Implementation of functions required by util.hpp
 * @file
 */

#include "base/util.hpp"

#include "base/log.hpp"

using minote::L;

namespace ppk::assert {

void implementation::throwException(const AssertionException& e)
{
	L.fail(R"(Assertion "%s" triggered on line %d in %s: %s)",
		e.expression(), e.line(), e.file(), e.what());
}

}
