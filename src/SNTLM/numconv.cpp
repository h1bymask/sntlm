#include "numconv.h"

#include <type_traits>

numconv_error::numconv_error(const std::string& s) 
	: std::runtime_error(s)
{
}

numconv_error::numconv_error(const char* s) 
	: std::runtime_error(s)
{
}

const char *const detail::printf_trait<char>::decfmt = 
	std::is_signed<char>::value ? printf_trait<signed char>::decfmt : printf_trait<unsigned char>::decfmt;
const char *const detail::printf_trait<char>::hexfmt = 
	std::is_signed<char>::value ? printf_trait<signed char>::hexfmt : printf_trait<unsigned char>::hexfmt;

const char *const detail::printf_trait<signed char>::decfmt = "%hd";
const char *const detail::printf_trait<signed char>::hexfmt = "%hx";
const char *const detail::printf_trait<unsigned char>::decfmt = "%hu";
const char *const detail::printf_trait<unsigned char>::hexfmt = "%hx";

const char *const detail::printf_trait<signed short int>::decfmt = "%hd";
const char *const detail::printf_trait<signed short int>::hexfmt = "%hx";
const char *const detail::printf_trait<unsigned short int>::decfmt = "%hu";
const char *const detail::printf_trait<unsigned short int>::hexfmt = "%hx";

const char *const detail::printf_trait<signed int>::decfmt = "%d";
const char *const detail::printf_trait<signed int>::hexfmt = "%x";
const char *const detail::printf_trait<unsigned int>::decfmt = "%u";
const char *const detail::printf_trait<unsigned int>::hexfmt = "%x";

const char *const detail::printf_trait<signed long int>::decfmt = "%ld";
const char *const detail::printf_trait<signed long int>::hexfmt = "%lx";
const char *const detail::printf_trait<unsigned long int>::decfmt = "%lu";
const char *const detail::printf_trait<unsigned long int>::hexfmt = "%lx";

const char *const detail::printf_trait<signed long long int>::decfmt = "%I64d";
const char *const detail::printf_trait<signed long long int>::hexfmt = "%I64x";
const char *const detail::printf_trait<unsigned long long int>::decfmt = "%I64u";
const char *const detail::printf_trait<unsigned long long int>::hexfmt = "%I64x";

bool detail::checksign(std::string::const_iterator& it, const std::string::const_iterator& eit, bool& hassign) {
	if (it == eit) { return false; }
	if ('-' == *it) { hassign = true; ++it; return true; }
	if ('+' == *it) { hassign = false; ++it; return true; }

	hassign = false;
	return true;
}

unsigned char detail::getdigit(std::string::const_iterator& it, const std::string::const_iterator& eit, numconv::numconv_base base) {
	char t = *it;
	if ((t >= '0') && (t <= '9')) { ++it; return static_cast<unsigned char>(t - '0'); }
	
	if (numconv::base_hex == base) {
		if ((t >= 'A') && (t <= 'F')) { ++it; return static_cast<unsigned char>(t - 'A' + 10); }
		if ((t >= 'a') && (t <= 'f')) { ++it; return static_cast<unsigned char>(t - 'a' + 10); }
	}

	return detail::wrongdigit;
}