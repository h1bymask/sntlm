#include "uniconv.h"
#include "numconv.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "win32crypto.h"
#include "win32sockets.h"
#include "ntlm.h"
#include "http.h"
#include "base64.h"


template <typename T>
bool testone(T x) {
	std::string s = boost::lexical_cast<std::string, T>(x);
	T a, b;
	bool lf, nf;

	try {
		a = boost::lexical_cast<T, std::string>(s);
		lf = false;
	}
	catch (boost::bad_lexical_cast&) {
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
	T u; u;  
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
		WS32 _______;

		TcpClientSocket socket("10.12.0.60", 8080);
		ntlm_request_t request("TMG", "PC0048");

		std::stringstream temp;
		temp << request;
		std::string temps = temp.str();
		
		std::string http_request = 
			"GET http://www.yandex.ru/ HTTP/1.1\r\n"
			"Host: www.yandex.ru\r\n"
			"Connection: close\r\n"
			"Proxy-Authorization: NTLM " +	base64_encode<std::string::const_iterator>(std::begin(temps), std::end(temps)) + "\r\n"
			"\r\n"
			;

		std::cout << http_request << "======END OF REQUEST======" << std::endl;
				

		std::vector<BYTE> buff(std::begin(http_request), std::end(http_request));
		
		socket.send(std::begin(buff), std::end(buff));

		//		std::vector<BYTE> response(8 * 1024, 0);
		//		auto last = std::end(response);
		//		for (auto it = std::begin(response), eit = std::end(response); it != last; last = it, it = socket.recv_upto(it, eit));
		HttpResponse response(socket);
		std::cout << narrow(widen(response.getStatusLine(), CP_UTF8), CP_OEMCP) << std::endl;
		auto headers = response.getHeaders();
		for (auto it = std::begin(headers), eit = std::end(headers); it != eit; ++it) {
			for (auto vit = std::begin(it->second), veit = std::end(it->second); vit != veit; ++vit) {
				std::cout << it->first << ": " << *vit << std::endl;
			}
		}
		std::cout << "======END OF HEADERS======" << std::endl;

		auto& buffer = response.getBuffer();
		if (response.getIsChunked()) {
			std::cout << "[[CHUNKED ENCODING]]" << std::endl;
			for (auto chunk = buffer.getchunk(); !chunk.empty(); chunk = buffer.getchunk()) {
				std::cout << chunk;
			}
			std::cout << "[[TRAILING HEADERS]]" << std::endl;
			for (auto th = buffer.getline(); !th.empty(); th = buffer.getline()) {
				std::cout << th << std::endl;
			}
		}
		else if (HttpResponse::nlen != response.getContentLength()) {
			std::cout << "[[EXACTLY " << response.getContentLength() << " BYTES]]" << std::endl;
			std::cout << buffer.getcount(response.getContentLength()) << std::endl;
		}
		else {
			buff.resize(8 * 1024, 0);
			for (auto last = socket.recv_upto(std::begin(buff), std::end(buff));
				last != std::begin(buff); last = socket.recv_upto(std::begin(buff), std::end(buff))) {
					std::cout << std::string(std::begin(buff), last);
			}
		}
		std::cout << std::endl << "======END OF RESPONSE======" << std::endl;

		auto proxyauth = headers.find("Proxy-Authenticate");
		if (std::end(headers) == proxyauth) {
			std::cout << "Expected a Proxy-Authenticate header in response" << std::endl;
			return 1;
		}
		if (proxyauth->second.size() > 1) {
			std::cout << "Multiple Proxy-Authenticate headers are not allowed" << std::endl;
			return 2;
		}
		if (proxyauth->second.begin()->substr(0, 5) != "NTLM ") {
			std::cout << "Expected NTLM token in Proxy-Authenticate header" << std::endl;
			return 3;
		}

		std::string tmp = proxyauth->second.begin()->substr(5);
		std::vector<BYTE> challenge = base64_decode(std::begin(tmp), std::end(tmp));
		std::cout << dump_memory(challenge);

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