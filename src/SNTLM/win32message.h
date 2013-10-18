#pragma once
#ifndef _WIN32_MESSAGE_H_
#define _WIN32_MESSAGE_H_

#include <Windows.h>

#include <string>

std::string GetWin32ErrorMessageA(DWORD error, UINT codepage = CP_UTF8);
std::wstring GetWin32ErrorMessageW(DWORD error);

#endif // _WIN32_MESSAGE_H_