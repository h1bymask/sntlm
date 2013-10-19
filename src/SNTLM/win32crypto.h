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
	~CryptoKey();

	HCRYPTKEY data() const;
private:
	CryptoKey();

	HCRYPTKEY key;
};

template <ALG_ID HashAlg>
class hmac_t {
public:
	hmac_t(HCRYPTPROV prov, const std::string& password)
		: key(prov, password)
	{
		WIN32_BOOLCHECKED(CryptCreateHash(prov, CALG_HMAC, key.data(), 0, &hash));

		HMAC_INFO hmacinfo;
		hmacinfo.pbInnerString = NULL;
		hmacinfo.cbInnerString = 0;
		hmacinfo.pbOuterString = NULL;
		hmacinfo.cbOuterString = 0;
		hmacinfo.HashAlgid = HashAlg;

		WIN32_BOOLCHECKED(CryptSetHashParam(hash, HP_HMAC_INFO, (const BYTE*)&hmacinfo, 0));
	}

	hmac_t(hmac_t&& old)
		: key(old.key)
		, hash(NULL)
	{
		std::swap(hash, old.hash);
	}

	~hmac_t() {
		CryptDestroyHash(hash);
	}

	hmac_t& hmac_t::append(const std::vector<BYTE>& data) {
		WIN32_BOOLCHECKED(CryptHashData(hash, data.data(), data.size(), 0));
		return (*this);
	}

	std::vector<BYTE> hmac_t::finish() {
		const DWORD MD5HASHLEN = 16;
		DWORD datalen = MD5HASHLEN;
		std::vector<BYTE> result(MD5HASHLEN, 0);
		WIN32_BOOLCHECKED(CryptGetHashParam(hash, HP_HASHVAL, &result[0], &datalen, 0));
		if (MD5HASHLEN != datalen) { throw win32_exception(NTE_BAD_HASH); }
		return result;
	}

private:
	hmac_t();
	hmac_t(const hmac_t&);	
	hmac_t operator=(const hmac_t&);

	CryptoKey key;
	HCRYPTHASH hash;
};

class CryptoProvider {
public:
	CryptoProvider();
	~CryptoProvider();

	hmac_t<CALG_MD5> new_hmac_md5(const std::string& password) {
		return hmac_t<CALG_MD5>(prov, password);
	}
private:
	CryptoProvider(const CryptoProvider&);
	CryptoProvider& operator=(const CryptoProvider&);

	HCRYPTPROV prov;
};

#endif // _WIN32_CRYPTO_H_