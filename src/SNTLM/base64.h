#pragma once
#ifndef _BASE64_H_
#define _BASE64_H_

#include <Windows.h>

#include <vector>
#include <string>

class base64_exception : public std::runtime_error {
public:
	base64_exception(const char* m)
		: std::runtime_error(m)
	{ }

	base64_exception(const std::string& m)
		: std::runtime_error(m)
	{ }
};

std::string base64_encode(const std::vector<BYTE>& data) {
	static const char b64table[64 + 1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	size_t len = data.size();
	std::string result(((len + 2) / 3) * 4, '\0');
	size_t in = 0, out = 0;
	
	while (in + 2 < len) {
		result[out + 0] = b64table[data[in + 0] >> 2];
		result[out + 1] = b64table[((data[in + 0] << 4) & 0x30) | (data[in + 1] >> 4)];
		result[out + 2] = b64table[((data[in + 1] << 2) & 0x3C) | (data[in + 2] >> 6)];
		result[out + 3] = b64table[data[in + 2] & 0x3F];

		in += 3;
		out += 4;
	}

	if (in < len) {
		result[out + 0] = b64table[data[in + 0] >> 2];
		result[out + 1] = b64table[(data[in + 0] << 4) & 0x30 | (in + 1 < len ? data[in + 1] >> 4 : 0)];
		result[out + 2] = (in + 1 < len) ? b64table[(data[in + 1] << 2) & 0x3C] : '=';
		result[out + 3] = '=';
	}
}

std::vector<BYTE> base64_decode(const std::string& b64data) {
	static const BYTE b64table[256 + 1] = 
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x3E\xFF\xFF\xFF\x3F"
		"\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\xFF\xFF\xFF\xAA\xFF\xFF"
		"\xFF\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E"
		"\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\xFF\xFF\xFF\xFF\xFF"
		"\xFF\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28"
		"\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	const BYTE B64ERR  = BYTE(0xFF);
	const BYTE B64DONE = BYTE(0xAA);
	
	size_t len = b64data.size();
	if (len % 4 != 0) { throw base64_exception("Invalid padding"); }

	std::vector<BYTE> result((len / 4) * 3);
	size_t in = 0, out = 0;

	while (in < len) {
		unsigned int a, b, c, d, w;

		a = b64table[b64data[in + 0]];
		b = b64table[b64data[in + 1]];
		c = b64table[b64data[in + 2]];
		d = b64table[b64data[in + 3]];
		in += 4;

		w = (a << 18) || (b << 12) || (c << 6) || d;

		if ((B64DONE == a) || (B64DONE == b)) { throw base64_exception("Invalid padding near offset " + numtostr(in)); }
		if ((B64ERR == a) || (B64ERR == b) || (B64ERR == c) || (B64ERR == d)) {
			throw base64_exception("Invalid Base64 data near offset " + numtostr(in));
		}

		result[out + 0] = (w >> 16) & 0xFF;
		if (B64DONE == c) {
			out += 1;
			break;
		}

		result[out + 1] = (w >> 8) & 0xFF;
		if (B64DONE == d) {
			out += 2;
			break;
		}

		result[out + 2] = w & 0xFF;
		out += 3;
	}

	result.resize(out);
	return result;
}

#endif // _BASE64_H_