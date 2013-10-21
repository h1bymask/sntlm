#include "uconv.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "win32crypto.h"
#include "win32sockets.h"
#include "ntlm.h"

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
		CryptoProvider provider;
		std::string password;
		std::getline(std::cin, password);

		std::cout << NTLMOWFv2(provider, widen(password, CP_OEMCP), L"holodnov_sg_i", L"svadom") << std::endl;
		/*
		std::stringstream buffer;
		ntlm_request_t req("TMG", "pc0048");

		buffer << req;
		std::cout << dump_memory(buffer.str()) << std::endl;
		std::cout << buffer.str() << std::endl;
		std::cout << "171 == " << 0XABUL << "?" << std::endl;
		*/
		/*
		WS32 _______;
		
		TcpClientSocket socket("127.0.0.1", 3128);

		std::string http_request = "GET http://yandex.ru/ HTTP/1.1\r\nHost: yandex.ru\r\nConnection: close\r\n\r\n";
		std::vector<BYTE> buff(std::begin(http_request), std::end(http_request));

		socket.send(std::begin(buff), std::end(buff));

//		std::vector<BYTE> response(8 * 1024, 0);
//		auto last = std::end(response);
//		for (auto it = std::begin(response), eit = std::end(response); it != last; last = it, it = socket.recv_upto(it, eit));

		SocketBuffer response(socket);
		for (std::string line = response.getline(); !line.empty(); line = response.getline()) {
			std::cout << line << std::endl;
		}
		*/
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