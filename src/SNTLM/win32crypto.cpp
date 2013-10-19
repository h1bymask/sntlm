#include "win32crypto.h"
#include "win32message.h"

//////////////////////////////////////////////////////////////////////////////
//
//	PlainText_KeyBLOB
//
//////////////////////////////////////////////////////////////////////////////

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


//////////////////////////////////////////////////////////////////////////////
//
//	CryptoKey
//
//////////////////////////////////////////////////////////////////////////////

CryptoKey::CryptoKey(HCRYPTPROV prov, const std::string& password)
	: key(NULL)
{
	PlainText_KeyBLOB keyblob(prov, password);
	WIN32_BOOLCHECKED(
		CryptImportKey(prov, keyblob.data(), keyblob.length(), 0, CRYPT_IPSEC_HMAC_KEY, &key));
}

CryptoKey::CryptoKey(const CryptoKey& right)
	: key(NULL)
{
	WIN32_BOOLCHECKED(CryptDuplicateKey(right.key, NULL, 0, &key));
}

CryptoKey::CryptoKey(CryptoKey&& old)
	: key(NULL)
{
	swap(*this, old);
}

void swap(CryptoKey& left, CryptoKey& right) {
	using std::swap;

	swap(left.key, right.key);
}

CryptoKey::~CryptoKey() {
	CryptDestroyKey(key);
}

CryptoKey& CryptoKey::operator=(CryptoKey right) {
	CryptoKey temp(right);
	swap(*this, temp);
	return (*this);
}

HCRYPTKEY CryptoKey::data() const { return key; }


//////////////////////////////////////////////////////////////////////////////
//
//	hmac_t
//
//////////////////////////////////////////////////////////////////////////////

hmac_t::hmac_t(HCRYPTPROV prov, ALG_ID hash_alg, const CryptoKey& ___key)
	: key(___key)
	, hash(NULL)
{
	WIN32_BOOLCHECKED(CryptCreateHash(prov, CALG_HMAC, key.data(), 0, &hash));

	HMAC_INFO hmacinfo;
	hmacinfo.pbInnerString = NULL;
	hmacinfo.cbInnerString = 0;
	hmacinfo.pbOuterString = NULL;
	hmacinfo.cbOuterString = 0;
	hmacinfo.HashAlgid = hash_alg;

	WIN32_BOOLCHECKED(CryptSetHashParam(hash, HP_HMAC_INFO, (const BYTE*)&hmacinfo, 0));
}

void swap(hmac_t& first, hmac_t& second) {
	using std::swap;

	swap(first.key, second.key);
	swap(first.hash, second.hash);
}

hmac_t::hmac_t(hmac_t&& old)
	: key(key)
	, hash(NULL)
{
	swap(*this, old);
}

hmac_t::hmac_t(const hmac_t& old)
	: key(key)
	, hash(NULL)
{
	WIN32_BOOLCHECKED(CryptDuplicateHash(old.hash, NULL, 0, &hash));
}

hmac_t& hmac_t::operator=(hmac_t right) {
	hmac_t temp(right);
	swap(*this, temp);
	return (*this);
}

hmac_t::~hmac_t() {
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

//////////////////////////////////////////////////////////////////////////////
//
//	CryptoProvider
//
//////////////////////////////////////////////////////////////////////////////

CryptoProvider::CryptoProvider()
	: prov(NULL)
{
	WIN32_BOOLCHECKED(CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, 0));
}

CryptoProvider::~CryptoProvider() {
	CryptReleaseContext(prov, 0);
}