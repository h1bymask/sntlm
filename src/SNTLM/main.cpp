#include "uconv.h"

#include <iostream>


int wmain (int argc, wchar_t **argv) {
	std::string s("");
	
	std::wcout << widen(s) << std::endl;

	return 0;
}