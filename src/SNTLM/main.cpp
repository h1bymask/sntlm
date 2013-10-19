#include "uconv.h"

#include <iostream>
#include <iomanip>
#include "win32crypto.h"

int wmain (int argc, wchar_t **argv) {
	win32_exception::setCodePage(CP_OEMCP);
	try {

		std::string password, data;

		std::cout << "HMAC-MD5" << std::endl;
		CryptoProvider provider;

		std::cout << "\tEnter key: ";
		std::getline(std::cin, password);

		do {
			std::cout << "\tEnter data: ";
			std::getline(std::cin, data);

			auto hash = provider.new_hmac_md5(password);
			std::vector<BYTE> result = hash.append(std::vector<BYTE>(std::begin(data), std::end(data))).finish();

			std::cout << "Hash:\t\t" << std::hex;
			for (auto it = std::begin(result), eit = std::end(result); it != eit; ++it) {
				std::cout << std::setw(2) << std::setfill('0') << (int)(*it);
			}
			std::cout << std::endl << "==============================" << std::endl;
		} while (true);
	}
	catch (win32_exception& e) {
		std::cout << e.what() << std::endl;
	}

	return 0;
}