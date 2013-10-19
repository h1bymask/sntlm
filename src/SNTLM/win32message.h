#pragma once
#ifndef _WIN32_MESSAGE_H_
#define _WIN32_MESSAGE_H_

#include <Windows.h>

#include <string>

class win32_exception : public std::runtime_error {
public:
	explicit win32_exception(DWORD errorcode);

	static void setCodePage(UINT codepage);

private:
	static UINT codepage;
	static std::string generateWhat(DWORD errorcode);
};

#define WIN32_BOOLCHECKED(c) do { if (!c) { DWORD err = GetLastError(); throw win32_exception(err); } } while(false);

std::string GetWin32ErrorMessageA(DWORD error, UINT codepage = CP_UTF8);
std::wstring GetWin32ErrorMessageW(DWORD error);

#endif // _WIN32_MESSAGE_H_