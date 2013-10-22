#pragma once
#ifndef _BASE64_H_
#define _BASE64_H_

#include <Windows.h>

#include <vector>
#include <string>

std::string base64_encode(const std::vector<BYTE>& data) {
	static const char const b64table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	size_t datasize = data.size(), b64count;
	b64count = datasize / 3;
	b64count += (datasize % 3) ? 1 : 0;

	std::string result(b64count * 4, '\0');

	auto cursor = std::begin(data); 
	size_t idx = 0;
	unsigned int temp = 0;

	for( ; idx < datasize / 3; idx++) {
		temp = 0;
		temp += (*cursor++) << 16;
		temp += (*cursor++) << 8;
		temp += (*cursor++);
		result[idx * 4 + 0] = b64table[(temp & 0x00FC0000) >> 18];
		result[idx * 4 + 1] = b64table[(temp & 0x0003F000) >> 12];
		result[idx * 4 + 2] = b64table[(temp & 0x00000FC0) >> 6 ];
		result[idx * 4 + 3] = b64table[(temp & 0x0000003F)];
	}

	switch(datasize % 3)
	{
	case 1:
		temp = (*cursor++) << 16;
		result[idx * 4 + 0] = b64table[(temp & 0x00FC0000) >> 18];
		result[idx * 4 + 1] = b64table[(temp & 0x0003F000) >> 12];
		result[idx * 4 + 2] = '=';
		result[idx * 4 + 3] = '=';
		break;
	case 2:
		temp = 0;
		temp += (*cursor++) << 16;
		temp += (*cursor++) << 8;
		result[idx * 4 + 0] = b64table[(temp & 0x00FC0000) >> 18];
		result[idx * 4 + 1] = b64table[(temp & 0x0003F000) >> 12];
		result[idx * 4 + 2] = b64table[(temp & 0x00000FC0) >> 6 ];
		result[idx * 4 + 3] = '=';
		break;
	}
	return result;
}

#endif // _BASE64_H_