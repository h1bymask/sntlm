#include "win32crypto.h"
#include "win32message.h"

class PlainText_KeyBLOB {
	struct data_t {
	public:
		BLOBHEADER header;
		DWORD keylen;
		BYTE keydata[0];
	};

public:
	PlainText_KeyBLOB(HCRYPTPROV prov, const std::string& key)
		: blob(NULL)
		, len(sizeof(data_t) + key.length())
	{
		blob = (data_t*)malloc(len);
		if (!blob) { throw std::bad_alloc(); }
		
		blob->header.aiKeyAlg = CALG_RC2;
		blob->header.bType = PLAINTEXTKEYBLOB;
		blob->header.bVersion = CUR_BLOB_VERSION;
		blob->header.reserved = 0;
		blob->keylen = key.length();
		memcpy(blob->keydata, key.data(), blob->keylen);		
	}

	~PlainText_KeyBLOB() {
		if (blob) { 
			SecureZeroMemory(blob->keydata, blob->keylen); 
			free(blob);
			blob = NULL;
		}
	}

	const BYTE* data() const { return (const BYTE*)blob; }

	DWORD length() const { return len; }

private:
	PlainText_KeyBLOB();
	PlainText_KeyBLOB(const PlainText_KeyBLOB&);
	PlainText_KeyBLOB(PlainText_KeyBLOB&&);
	PlainText_KeyBLOB& operator=(const PlainText_KeyBLOB&);

	data_t *blob;
	DWORD len;
};

CryptoKey::CryptoKey(HCRYPTPROV prov, const std::string& password) {
	PlainText_KeyBLOB keyblob(prov, password);
	WIN32_BOOLCHECKED(
		CryptImportKey(prov, keyblob.data(), keyblob.length(), 0, CRYPT_IPSEC_HMAC_KEY, &key));
}

CryptoKey::~CryptoKey() {
	CryptDestroyKey(key);
}

HCRYPTKEY CryptoKey::data() const { return key; }

hmac_md5_t::hmac_md5_t(HCRYPTPROV prov, const std::string& password) 	
	: key(prov, password)
{
	WIN32_BOOLCHECKED(CryptCreateHash(prov, CALG_HMAC, key.data(), 0, &hash));
}

hmac_md5_t::~hmac_md5_t() {
	CryptDestroyHash(hash);
}

hmac_md5_t& hmac_md5_t::append(const std::vector<BYTE>& data) {
	WIN32_BOOLCHECKED(CryptHashData(hash, data.data(), data.size(), 0));
	return (*this);
}

std::vector<BYTE> hmac_md5_t::finish() {
	const DWORD MD5HASHLEN = 16;
	DWORD datalen = MD5HASHLEN;
	std::vector<BYTE> result(MD5HASHLEN, 0);
	WIN32_BOOLCHECKED(CryptGetHashParam(hash, HP_HASHVAL, &result[0], &datalen, 0));
	if (MD5HASHLEN != datalen) { throw win32_exception(NTE_BAD_HASH); }
	return result;
}

CryptoProvider::CryptoProvider() {
	if (!CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, 0)) {
		DWORD error = GetLastError();
		throw win32_exception(error);
	}
}

CryptoProvider::~CryptoProvider() {
	CryptReleaseContext(prov, 0);
}
