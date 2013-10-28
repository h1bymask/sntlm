#include "uniconv.h"
#include "numconv.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <iterator>

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

void print_response(HttpResponse& response, TcpClientSocket& socket) {
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
		std::vector<BYTE> buff(8 * 1024, 0);
		for (auto last = socket.recv_upto(std::begin(buff), std::end(buff));
			last != std::begin(buff); last = socket.recv_upto(std::begin(buff), std::end(buff))) {
				std::cout << std::string(std::begin(buff), last);
		}
	}
	std::cout << std::endl << "======END OF RESPONSE======" << std::endl;
}

int wmain (int argc, wchar_t **argv) {
	win32_exception::setCodePage(CP_OEMCP);
	try {
		WS32 _______;

		std::wstring domain(L"TMG"), hostname(L"PC0048");

		TcpClientSocket socket("10.12.0.60", 8080);
		ntlm_request_t request(narrow(domain), narrow(hostname));

		std::stringstream temp;
		temp << request;
		std::string temps = temp.str();

		std::string http_request_start = 
			"GET http://www.yandex.ru/ HTTP/1.1\r\n"
			"Host: www.yandex.ru\r\n"
			//"Connection: close\r\n"
			;

		std::string proxy_auth = "Proxy-Authorization: NTLM ";

		std::string http_request;
		http_request 
			= http_request_start 
			+ proxy_auth 
			+ base64_encode(std::begin(temps), std::end(temps)) 
			+ "\r\n\r\n";

		std::cout << http_request << "======END OF REQUEST======" << std::endl;

		socket.send(std::begin(http_request), std::end(http_request));

		//		std::vector<BYTE> response(8 * 1024, 0);
		//		auto last = std::end(response);
		//		for (auto it = std::begin(response), eit = std::end(response); it != last; last = it, it = socket.recv_upto(it, eit));
		HttpResponse response(socket);

		print_response(response, socket);

		auto headers = response.getHeaders();

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

		size_t challen = challenge.size();
		if (challen <= 48) {
			std::cout << "NTLM challenge message is too short" << std::endl;
			return 4;
		}

		unsigned short tbofs, tpos, ttype = 0, tblen = 0;

		tbofs = tpos = challenge[44] | (challenge[45] << 8);
		while (tpos + 4 <= challen && (ttype = challenge[tpos] | (challenge[tpos + 1] << 8))) {
			unsigned short tlen = challenge[tpos + 2] | (challenge[tpos + 3] << 8);
			if (tpos + 4 + tlen > challen)
				break;

			tpos += 4 + tlen;
			tblen += 4 + tlen;
		}

		if (tblen && ttype == 0)
			tblen += 4;

		CryptoProvider provider;

		std::string username, password;


		std::cout << "Enter the username: ";
		std::getline(std::cin, username);
		std::cout << "Enter the password: ";
		std::getline(std::cin, password);

		std::wstring wusername(widen(username, CP_OEMCP)), wpassword(widen(password, CP_OEMCP));
		for (auto it = std::begin(password), eit = std::end(password); it != eit; ++it) {
			*it = '\0';
		}

		auto ntlmowfv2 = NTLMOWFv2(provider, wpassword, wusername, domain);

		SYSTEMTIME snow;
		FILETIME fnow;
		std::vector<BYTE> nonce(8, 0);
		GetSystemTime(&snow);
		SystemTimeToFileTime(&snow, &fnow);
		*(DWORD*)&nonce[0] = rand();
		*(DWORD*)&nonce[4] = rand();

		std::vector<BYTE> blob(4 + 4 + 8 + 8 + 4 + tblen + 4);
		*(DWORD*)&blob[0] = 0x00000101;
		*(DWORD*)&blob[4] = 0;
		*(DWORD*)&blob[8] = fnow.dwLowDateTime;
		*(DWORD*)&blob[12] = fnow.dwHighDateTime;
		*(DWORD*)&blob[16] = *(DWORD*)&nonce[0];
		*(DWORD*)&blob[20] = *(DWORD*)&nonce[4];
		*(DWORD*)&blob[24] = 0;
		memcpy(&blob[28], &challenge[tbofs], tblen);
		*(DWORD*)&blob[28 + tblen] = 0;

		std::vector<BYTE> server_flags(&challenge[24], &challenge[24 + 8]);

		std::vector<BYTE> nthash = provider.new_hmac_md5(ntlmowfv2).append(server_flags).append(blob).finish(),
			lmhash = provider.new_hmac_md5(ntlmowfv2).append(server_flags).append(nonce).finish();

		std::copy(std::begin(blob), std::end(blob), std::back_inserter(nthash));
		std::copy(std::begin(nonce), std::end(nonce), std::back_inserter(lmhash));

		DWORD dlen = 2 * domain.length(), ulen = 2 * wusername.length(), hlen = 2 * hostname.length();

		std::vector<BYTE> ntlm_resp(64 + dlen + ulen + hlen + lmhash.size() + nthash.size());
		memcpy(&ntlm_resp[0], "NTLMSSP\0", 8);
		*(DWORD*)&ntlm_resp[8] = 3;

		*(WORD*)&ntlm_resp[12] = lmhash.size();
		*(WORD*)&ntlm_resp[14] = lmhash.size();
		*(DWORD*)&ntlm_resp[16] = 64 + dlen + ulen + hlen;

		*(WORD*)&ntlm_resp[20] = nthash.size();
		*(WORD*)&ntlm_resp[22] = nthash.size();
		*(DWORD*)&ntlm_resp[24] = 64 + dlen + ulen + hlen + lmhash.size();

		*(WORD*)&ntlm_resp[28] = dlen;
		*(WORD*)&ntlm_resp[30] = dlen;
		*(DWORD*)&ntlm_resp[32] = 64;

		*(WORD*)&ntlm_resp[36] = ulen;
		*(WORD*)&ntlm_resp[38] = ulen;
		*(DWORD*)&ntlm_resp[40] = 64 + dlen;

		*(WORD*)&ntlm_resp[44] = hlen;
		*(WORD*)&ntlm_resp[46] = hlen;
		*(WORD*)&ntlm_resp[48] = 64 + dlen + ulen;

		/* Session */
		*(WORD*)&ntlm_resp[52] = 0;
		*(WORD*)&ntlm_resp[54] = 0;
		*(DWORD*)&ntlm_resp[56] = 64 + dlen + ulen + hlen + lmhash.size() + nthash.size();

		/* Flags */
		*(DWORD*)&ntlm_resp[60] = *(DWORD*)&challenge[20];

		memcpy(&ntlm_resp[64], domain.data(), dlen);
		memcpy(&ntlm_resp[64 + dlen], wusername.data(), ulen);
		memcpy(&ntlm_resp[64 + dlen + ulen], hostname.data(), hlen);
		memcpy(&ntlm_resp[64 + dlen + ulen + hlen], lmhash.data(), lmhash.size());
		memcpy(&ntlm_resp[64 + dlen + ulen + hlen + lmhash.size()], nthash.data(), nthash.size());

		std::string z = base64_encode(ntlm_resp.begin(), ntlm_resp.end());

		http_request 
			= http_request_start 
			+ proxy_auth 
			+ z + "\r\n\r\n";

		std::cout << http_request << "======END OF REQUEST======" << std::endl;

		socket.send(http_request.begin(), http_request.end());

		HttpResponse second_resp(socket);
		print_response(second_resp, socket);

		{
			http_request = "GET http://mail.ru/ HTTP/1.1\r\nHost: mail.ru\r\n\r\n";
			socket.send(http_request.begin(), http_request.end());
			HttpResponse r(socket);
			print_response(r, socket);
		}
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return 0;
}