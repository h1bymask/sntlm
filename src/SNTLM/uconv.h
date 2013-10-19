#pragma once
#ifndef _UCONV_H_
#define _UCONV_H_

#include <Windows.h>

#include <string>
#include <stdexcept>
#include <type_traits>

class uconv_error : public std::runtime_error {
public:
	explicit uconv_error(const std::string& s);
	explicit uconv_error(const char* s);
};

std::wstring widen(const std::string& string, UINT codepage = CP_UTF8);
std::wstring widen(const char *string, size_t length, UINT codepage = CP_UTF8);

std::string narrow(const std::wstring& string, UINT codepage = CP_UTF8);
std::string narrow(const wchar_t *string, size_t length, UINT codepage = CP_UTF8);

void toUpper(std::string& string, UINT codepage = CP_UTF8);
void toLower(std::string& string, UINT codepage = CP_UTF8);

void toUpper(std::wstring& string);
void toLower(std::wstring& string);



std::string numtostr(DWORD num);
std::string numtostr(size_t num);

#endif //_UCONV_H_