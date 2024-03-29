#include "uniconv.h"
#include "numconv.h"

#include <memory>
#include <algorithm>

uniconv_error::uniconv_error(const std::string& s)
	: std::runtime_error(s)
{ }

uniconv_error::uniconv_error(const char* s) 
	: std::runtime_error(s)
{ }


int MB2WC(UINT codepage, DWORD flags, const char* in, DWORD inlen, wchar_t* out, DWORD outlen) {
	return MultiByteToWideChar(codepage, flags, in, inlen, out, outlen);
}

int WC2MB(UINT codepage, DWORD flags, const wchar_t* in, DWORD inlen, char* out, DWORD outlen) {
	return WideCharToMultiByte(codepage, flags, in, inlen, out, outlen, NULL, NULL);
}


template <typename ConvFunc, typename InChar, typename OutStr>
OutStr doconversion(const InChar *string, size_t length, UINT codepage, ConvFunc conv) {
	if (((DWORD)-1 == (DWORD)length) || (length > MAXDWORD)) {
		throw std::invalid_argument("String is too long");
	}

	if (!length) {
		return OutStr();
	}

	int reqlen = conv(codepage, 0, string, length, NULL, 0);
	if (!reqlen) {
		DWORD error = GetLastError();
		throw uniconv_error("Conversion error: " + numtostr(error));
	}

	OutStr result(reqlen, L'\0');
	int len = conv(codepage, 0, string, length, &result[0], reqlen);

	if (!len) {
		DWORD error = GetLastError();
		throw uniconv_error("Conversion error: " + numtostr(error));
	}
	if (len != reqlen) {
		throw uniconv_error("String was misconverted");
	}

	return result;
}



std::wstring widen(const std::string& string, UINT codepage) {
	return widen(string.c_str(), string.length(), codepage);
}

std::wstring widen(const char *string, size_t length, UINT codepage) {
	return doconversion<decltype(&MB2WC), char, std::wstring>(string, length, codepage, MB2WC);
}

std::string narrow(const std::wstring& string, UINT codepage) {
	return narrow(string.c_str(), string.length(), codepage);
}

std::string narrow(const wchar_t *string, size_t length, UINT codepage) {
	return doconversion<decltype(&WC2MB), wchar_t, std::string>(string, length, codepage, WC2MB);
}


template <typename Func>
void inplaceW(std::wstring& string, Func func) {
	if (string.length() > MAXDWORD) {
		throw uniconv_error("String too long");
	}
	func(&string[0], string.length());
}

template <typename Func>
void inplaceA(std::string& string, UINT codepage, Func func) {
	std::wstring temp = widen(string, codepage);
	inplaceW(temp, func);
	string = narrow(temp, codepage);
}

void toUpper(std::string& string, UINT codepage) {
	return inplaceA(string, codepage, CharUpperBuffW);
}

void toLower(std::string& string, UINT codepage) {
	return inplaceA(string, codepage, CharLowerBuffW);
}

void toUpper(std::wstring& string) {
	return inplaceW(string, CharUpperBuffW);
}

void toLower(std::wstring& string) {
	return inplaceW(string, CharLowerBuffW);
}


bool ishtsp(char c) {
	return ('\x20' == c) || ('\t' == c);
}
	
void trim(std::string& s) {
	s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), ishtsp));
	s.erase(std::find_if_not(s.rbegin(), s.rend(), ishtsp).base(), s.end());
}
