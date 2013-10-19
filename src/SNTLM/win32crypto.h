#pragma once
#ifndef _WIN32_CRYPTO_H_
#define _WIN32_CRYPTO_H_

#include <Windows.h>
#include <WinCrypt.h>

#include <vector>


class CryptoKey {
public:
	CryptoKey(HCRYPTPROV prov, const std::string& password);
	~CryptoKey();

	HCRYPTKEY data() const;
private:
	CryptoKey();
	CryptoKey(const CryptoKey&);
	CryptoKey(CryptoKey&&);
	CryptoKey& operator=(const CryptoKey&);

	HCRYPTKEY key;
};

class hmac_md5_t {
public:
	hmac_md5_t(HCRYPTPROV prov, const std::string& password);
	~hmac_md5_t();

	hmac_md5_t& append(const std::vector<BYTE>& data);
	std::vector<BYTE> finish();

private:
	hmac_md5_t();
	hmac_md5_t(const hmac_md5_t&);
	hmac_md5_t(hmac_md5_t&&);
	hmac_md5_t operator=(const hmac_md5_t&);

	CryptoKey key;
	HCRYPTHASH hash;
};

class CryptoProvider {
public:
	CryptoProvider();
	~CryptoProvider();
			
private:
	HCRYPTPROV prov;
};

#endif // _WIN32_CRYPTO_H_