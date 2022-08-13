#ifndef UTIL_HLSLI
#define UTIL_HLSLI

template<typename T>
T divRoundUp(T _n, T _div) {
	
	return (_n + (_div - 1)) / _div;
	
}

uint bitmask(uint _bits) {
	
	return (1u << _bits) - 1u;
	
}

#endif
