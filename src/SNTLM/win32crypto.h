#pragma once
#ifndef _WIN32_CRYPTO_H_
#define _WIN32_CRYPTO_H_

#include <Windows.h>
#include <WinCrypt.h>

#include <vector>

#include "win32message.h"

class CryptoKey {
public:
	CryptoKey(HCRYPTPROV prov, const std::string& password);
	CryptoKey(const CryptoKey& right);
	CryptoKey(CryptoKey&& old);
	friend void swap(CryptoKey& left, CryptoKey& right);
	~CryptoKey();

	CryptoKey& operator=(CryptoKey right);

	HCRYPTKEY data() const;
private:
	CryptoKey();

	HCRYPTKEY key;
};

class hmac_t {
public:
	hmac_t(HCRYPTPROV prov, ALG_ID hash_alg, const CryptoKey& key);
	friend void swap(hmac_t& first, hmac_t& second);
	hmac_t(hmac_t&& old);
	hmac_t(const hmac_t& old);
	hmac_t& operator=(hmac_t right);
	~hmac_t();

	hmac_t& append(const std::vector<BYTE>& data);
	std::vector<BYTE> finish();

private:
	hmac_t();

	CryptoKey key;
	HCRYPTHASH hash;
};

class CryptoProvider {
public:
	CryptoProvider();
	~CryptoProvider();

	hmac_t new_hmac_md5(const std::string& password) {
		return hmac_t(prov, CALG_MD5, CryptoKey(prov, password));
	}
private:
	CryptoProvider(const CryptoProvider&);
	CryptoProvider& operator=(const CryptoProvider&);

	HCRYPTPROV prov;
};

#endif // _WIN32_CRYPTO_H_