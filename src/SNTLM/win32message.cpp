#include "win32message.h"
#include "uniconv.h"
#include "numconv.h"

#include <memory>

UINT win32_exception::codepage;

win32_exception::win32_exception(DWORD errorcode) 
	: std::runtime_error(generateWhat(errorcode))
{ }	

void win32_exception::setCodePage(UINT codepage) {
	win32_exception::codepage = codepage;
}

std::string win32_exception::generateWhat(DWORD errorcode) {
	std::string result = GetWin32ErrorMessageA(errorcode, codepage);
	result += " (";
	result += numtostr(errorcode);
	result += ")";
	return result;
}


std::string GetWin32ErrorMessageA(DWORD error, UINT codepage) {
	try {
		return narrow(GetWin32ErrorMessageW(error), codepage);
	}
	catch (uniconv_error&) {
		return std::string();
	}
}

std::wstring GetWin32ErrorMessageW(DWORD error) {
    LPWSTR buffer = NULL;

    DWORD buffcount = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&buffer,
        0,
        NULL
    );
    
    if (NULL != buffer) {
		std::unique_ptr<wchar_t, decltype(&::LocalFree)> buffer_(buffer, ::LocalFree);
		if ((buffcount >= 2) && 
			(L'\r' == buffer[buffcount - 2]) && 
			(L'\n' == buffer[buffcount - 1])) {
				buffcount -= 2;
		}
        std::wstring result(buffer, buffcount);
        return result;
    }
    
    return std::wstring();
}