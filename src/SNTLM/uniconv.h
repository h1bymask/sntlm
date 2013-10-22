#pragma once
#ifndef _UNI_CONV_H_
#define _UNI_ONV_H_

#include <Windows.h>

#include <string>
#include <type_traits>

class uniconv_error : public std::runtime_error {
public:
	explicit uniconv_error(const std::string& s);
	explicit uniconv_error(const char* s);
};

std::wstring widen(const std::string& string, UINT codepage = CP_UTF8);
std::wstring widen(const char *string, size_t length, UINT codepage = CP_UTF8);

std::string narrow(const std::wstring& string, UINT codepage = CP_UTF8);
std::string narrow(const wchar_t *string, size_t length, UINT codepage = CP_UTF8);

void toUpper(std::string& string, UINT codepage = CP_UTF8);
void toLower(std::string& string, UINT codepage = CP_UTF8);

void toUpper(std::wstring& string);
void toLower(std::wstring& string);

bool ishtsp(char c);
void trim(std::string& s);

#endif //_UNI_CONV_H_
