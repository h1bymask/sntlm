#pragma once
#ifndef _NUM_CONV_H_
#define _NUM_CONV_H_

#include <string>
#include <type_traits>

class numconv_error : public std::runtime_error {
public:
	explicit numconv_error(const std::string& s);
	explicit numconv_error(const char* s);
};

namespace numconv {
	enum numconv_base {
		base_dec, 
		base_hex
	};
}

namespace detail {
	template<typename T>
	struct printf_trait {
		static const char *const decfmt, *const hexfmt;
	};
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value 
	&& std::is_same<T, typename std::remove_cv<T>::type>::value
	&& !std::is_same<T, bool>::value,
std::string>::type 
#pragma warning(push)
#pragma warning(disable:4996)
numtostr(T num, numconv::numconv_base base = numconv::base_dec) {
	using namespace detail;
	std::string result(std::numeric_limits<T>::digits10 + 2, '\0'); // +1 for possible sign, +1 because of what digits10 is
	switch (base) {
	case numconv::base_dec:
		result.resize(sprintf(&result[0], printf_trait<T>::decfmt, num));
		break;
	case numconv::base_hex:
		result.resize(sprintf(&result[0], printf_trait<T>::hexfmt, num));
		break;
	default:
		throw numconv_error("Invalid base specified");
	}
	return result;
}
#pragma warning(pop)

namespace detail {
	bool checksign(std::string::const_iterator& it, const std::string::const_iterator& eit, bool& hassign);
	unsigned char getdigit(std::string::const_iterator& it, const std::string::const_iterator& eit, numconv::numconv_base base);

	const unsigned char wrongdigit = static_cast<unsigned char>(-1);
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value 
	&& std::is_same<T, typename std::remove_cv<T>::type>::value
	&& !std::is_same<T, bool>::value,
bool>::type 
strtonum(const std::string& s, T& num, numconv::numconv_base base = numconv::base_dec) {
	using namespace detail;
	typedef std::make_unsigned<T>::type UT;
	UT numbase = 0;

	switch (base) {
	case numconv::base_dec:
		numbase = 10;
		break;
	case numconv::base_hex:
		numbase = 16;
		break;
	default:
		return false;
	}

	UT result = 0;
	bool hassign = false;

	auto it = s.begin(), eit = s.end();
	if (!checksign(it, eit, hassign)) { return false; }

	if (it == eit) { return false; }

	if (std::is_unsigned<T>::value && hassign) { return false; }

	while (it != eit) {
		UT digit = getdigit(it, eit, base);
		if (detail::wrongdigit == digit) { return false; }
		UT sub_result = result * numbase;
		if (sub_result / numbase != result) { return false; }
		result = sub_result + digit;
	}

	if (std::is_unsigned<T>::value) { num = result; return true; }

	if (hassign) {
#pragma warning(push)
#pragma warning(disable:4146)
		UT limit = static_cast<UT>(-(std::numeric_limits<T>::min()));
		if (result > limit) { return false; }
		num = static_cast<T>(-result);
#pragma warning(pop)
		return true;
	}
	else {
		UT limit = static_cast<UT>(std::numeric_limits<T>::max());
		if (result > limit) { return false; }
		num = static_cast<T>(result);
		return true;
	}
}

#endif //_NUM_CONV_H_