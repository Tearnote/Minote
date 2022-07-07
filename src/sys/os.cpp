#include "sys/os.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#include <windows.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <io.h>
#endif //_WIN32

namespace minote {

void OS::initConsole() {
	
#ifdef _WIN32 // Only Windows is supported right now
	
	// https://github.com/ocaml/ocaml/issues/9252#issuecomment-576383814
	AllocConsole();
	
	int fdOut = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)), _O_WRONLY | _O_BINARY);
	int fdErr = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_ERROR_HANDLE)),  _O_WRONLY | _O_BINARY);
	
	if (fdOut) {
		
		_dup2(fdOut, STDOUT_FILENO);
		_close(fdOut);
		SetStdHandle(STD_OUTPUT_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(STDOUT_FILENO)));
		
	}
	if (fdErr) {
		
		_dup2(fdErr, STDERR_FILENO);
		_close(fdErr);
		SetStdHandle(STD_ERROR_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(STDERR_FILENO)));
		
	}
	
	*stdout = *fdopen(STDOUT_FILENO, "wb");
	*stderr = *fdopen(STDERR_FILENO, "wb");
	
	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);
	
	// Set console encoding to UTF-8
	SetConsoleOutputCP(65001);
	
#endif //_WIN32
	
}

}
