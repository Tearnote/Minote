#pragma once

#include <streambuf>
#include <istream>
#include <ios>

namespace minote::base {

// https://github.com/ddiakopoulos/tinyply/blob/96379c8e15c4f02e83c720d3ac987532b6deb530/source/example-utils.hpp#L39

struct memory_buffer: public std::streambuf {
	char * p_start {nullptr};
	char * p_end {nullptr};
	size_t size;

	memory_buffer(char const * first_elem, size_t size)
		: p_start(const_cast<char*>(first_elem)), p_end(p_start + size), size(size)
	{
		setg(p_start, p_start, p_end);
	}

	pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode) override
	{
		if (dir == std::ios_base::cur) gbump(static_cast<int>(off));
		else setg(p_start, (dir == std::ios_base::beg ? p_start : p_end) + off, p_end);
		return gptr() - p_start;
	}

	pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
	{
		return seekoff(pos, std::ios_base::beg, which);
	}
};

struct memory_stream: virtual memory_buffer, public std::istream {
	memory_stream(char const * first_elem, size_t size)
		: memory_buffer(first_elem, size), std::istream(static_cast<std::streambuf*>(this)) {}
};

}
