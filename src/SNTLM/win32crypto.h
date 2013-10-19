#pragma once
#ifndef _WIN32_CRYPTO_H_
#define _WIN32_CRYPTO_H_

#include <Windows.h>
#include <WinCrypt.h>

#include <vector>

#include "win32message.h"


class CryptoKey {
public:
	CryptoKey();
	CryptoKey(HCRYPTPROV prov, const std::vector<BYTE>& password);
	CryptoKey(const CryptoKey& right);
	CryptoKey(CryptoKey&& old);
	friend void swap(CryptoKey& left, CryptoKey& right);
	~CryptoKey();

	CryptoKey& operator=(CryptoKey right);

	HCRYPTKEY data() const;

private:
	HCRYPTKEY key;
};


class hash_t {
public:
	hash_t(HCRYPTPROV prov, ALG_ID hash_alg, bool as_hmac, const CryptoKey& key);
	friend void swap(hash_t& first, hash_t& second);
	hash_t(hash_t&& old);
	hash_t(const hash_t& old);
	hash_t& operator=(hash_t right);
	~hash_t();

	hash_t& append(const std::vector<BYTE>& data);
	std::vector<BYTE> finish();

private:
	hash_t();

	CryptoKey key;
	HCRYPTHASH hash;
	DWORD hash_len;
};


class CryptoProvider {
public:
	CryptoProvider();
	friend void swap(CryptoProvider& left, CryptoProvider& right);
	CryptoProvider(CryptoProvider&&);
	~CryptoProvider();

	hash_t new_hmac_md5(const std::vector<BYTE>& password) const;
	hash_t new_md4() const;

private:
	CryptoProvider(const CryptoProvider&);	
	CryptoProvider& operator=(const CryptoProvider);

	HCRYPTPROV prov;
};


template <typename VectorLike>
std::vector<BYTE> dump_memory(const VectorLike& v) {
	return std::vector<BYTE>((const BYTE*)v.data(), (const BYTE*)(v.data() + v.size()));
}

#endif // _WIN32_CRYPTO_H_