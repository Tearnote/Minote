#include "base/util.hpp"

#include "base/log.hpp"

namespace ppk::assert {

// Implementation of the assert failure callback
void implementation::throwException(AssertionException const& e)
{
	minote::L.fail(R"(Assertion "{}" triggered on line {} in {}{}{})",
		e.expression(), e.line(), e.file(), std::strlen(e.what())? ": " : "", e.what());
}

}
