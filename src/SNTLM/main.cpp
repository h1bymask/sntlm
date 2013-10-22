#include "uniconv.h"
#include "numconv.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "win32crypto.h"
#include "win32sockets.h"
#include "ntlm.h"
#include "http.h"
#include <boost\lexical_cast.hpp>

template <typename T>
bool testone(T x) {
	std::string s = boost::lexical_cast<std::string, T>(x);
	T a, b;
	bool lf, nf;

	try {
		a = boost::lexical_cast<T, std::string>(s);
		lf = false;
	}
	catch (boost::bad_lexical_cast& e) {
		lf = true;
	}

	nf = !strtonum<T>(s, b);

	if (lf && !nf) {
		std::cout << ": lexical_cast failed, but not numtostr, which returned " << b << std::endl;
		return false;
	}
	else if (nf && !lf) {
		std::cout << ": numtostr failed, but not lexical_cast, which returned " << a << std::endl;
		return false;
	}
	else if (a != b) {
		std::cout << x << ": lexical_cast returned " << a << " but numtostr returned " << b << std::endl;
		return false;
	}

	return true;
}

template <typename T>
bool test() {
	T u;
	std::cout << "Testing " << typeid(u).name() << "..." << std::endl;
	bool not_yet = true, result = true;

	for (unsigned short i = 0; i <= 10; ++i) {
		result = result && testone(std::numeric_limits<T>::min() + i);
	}
	for (unsigned short i = 0; i <= 10; ++i) {
		result = result && testone(std::numeric_limits<T>::max() - i);
	}
	for (short i = -10; i <= 10; ++i) {
		result = result && testone(i);
	}

	return result;
}

std::ostream& operator<<(std::ostream& s, const std::vector<BYTE>& value) {
	struct z {
		z(std::ostream& s) : str(s), fl(s.flags()) { }
		~z() { str.flags(fl); }
		std::ostream& str;
		std::ios::fmtflags fl;
	} _z(s);

	s << std::hex << std::uppercase;
	for (auto it = std::begin(value), eit = std::end(value); it != eit; ++it) {
		s << std::setw(2) << std::setfill('0') << (0xFF ^ (int)(*it));
	}
	return s;
}

int wmain (int argc, wchar_t **argv) {
	win32_exception::setCodePage(CP_OEMCP);
	try {
		std::cout << std::boolalpha;
		std::cout << test<short>() << std::endl;
		std::cout << test<signed short>() << std::endl;
		std::cout << test<unsigned short>() << std::endl;
		std::cout << test<int>() << std::endl;
		std::cout << test<signed int>() << std::endl;
		std::cout << test<unsigned int>() << std::endl;
		std::cout << test<long>() << std::endl;
		std::cout << test<signed long>() << std::endl;
		std::cout << test<unsigned long>() << std::endl;
		std::cout << test<long long>() << std::endl;
		std::cout << test<signed long long>() << std::endl;
		std::cout << test<unsigned long long>() << std::endl;

		return 0;

		/*
		CryptoProvider provider;
		std::string password;
		std::getline(std::cin, password);

		std::cout << NTLMOWFv2(provider, widen(password, CP_OEMCP), L"holodnov_sg_i", L"svadom") << std::endl;
		*/
		/*
		std::stringstream buffer;
		ntlm_request_t req("TMG", "pc0048");

		buffer << req;
		std::cout << dump_memory(buffer.str()) << std::endl;
		std::cout << buffer.str() << std::endl;
		std::cout << "171 == " << 0XABUL << "?" << std::endl;
		*/

		WS32 _______;

		TcpClientSocket socket("10.12.0.60", 8080);

		std::string http_request = 
			"GET http://www.yandex.ru/ HTTP/1.1\r\n"
			"Host: www.yandex.ru\r\n"
			"Connection: close\r\n"
			"\r\n"
			;

		std::vector<BYTE> buff(std::begin(http_request), std::end(http_request));

		socket.send(std::begin(buff), std::end(buff));

		//		std::vector<BYTE> response(8 * 1024, 0);
		//		auto last = std::end(response);
		//		for (auto it = std::begin(response), eit = std::end(response); it != last; last = it, it = socket.recv_upto(it, eit));
		HttpResponse response(socket);
		std::cout << narrow(widen(response.getStatusLine(), CP_UTF8), CP_OEMCP) << std::endl;
		auto headers = response.getHeaders();
		for (auto it = std::begin(headers), eit = std::end(headers); it != eit; ++it) {
			std::cout << it->first << ": " << it->second << std::endl;
		}		
		std::cout << "======END OF HEADERS======" << std::endl;

		std::cout << response.getBuffer().getbuffer();
		std::cout << "||" << std::endl;

		buff.resize(8 * 1024, 0);
		for (auto last = socket.recv_upto(std::begin(buff), std::end(buff));
			last != std::begin(buff); last = socket.recv_upto(std::begin(buff), std::end(buff))) {
				std::cout << std::string(std::begin(buff), last);
		}
		std::cout << std::endl << "======END OF RESPONSE======" << std::endl;

		/*
		std::string password, data;

		std::cout << "HMAC-MD5" << std::endl;
		CryptoProvider provider;

		std::cout << "\tEnter key: ";
		std::getline(std::cin, password);

		do {
		std::cout << "\tEnter data: ";
		std::getline(std::cin, data);

		auto hash = provider.new_hmac_md5(std::vector<BYTE>(std::begin(password), std::end(password)));
		std::vector<BYTE> result = hash.append(std::vector<BYTE>(std::begin(data), std::end(data))).finish();

		std::cout << "Hash:\t\t" << std::hex;
		for (auto it = std::begin(result), eit = std::end(result); it != eit; ++it) {
		std::cout << std::setw(2) << std::setfill('0') << (int)(*it);
		}
		std::cout << std::endl << std::string(8 + 8 + 16 * 2, '=') << std::endl;
		} while (true);*/
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return 0;
}