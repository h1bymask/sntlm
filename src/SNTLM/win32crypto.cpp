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
	PlainText_KeyBLOB(HCRYPTPROV prov, const std::vector<BYTE>& key)
		: blob(NULL)
		, len(sizeof(data_t) + key.size())
	{
		blob = (data_t*)malloc(len);
		if (!blob) { throw std::bad_alloc(); }

		blob->header.aiKeyAlg = CALG_RC2;
		blob->header.bType = PLAINTEXTKEYBLOB;
		blob->header.bVersion = CUR_BLOB_VERSION;
		blob->header.reserved = 0;
		blob->keylen = key.size();
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

CryptoKey::CryptoKey() 
	: key(NULL)
{ }

CryptoKey::CryptoKey(HCRYPTPROV prov, const std::vector<BYTE>& password)
	: key(NULL)
{
	PlainText_KeyBLOB keyblob(prov, password);
	WIN32_BOOLCHECKED(
		CryptImportKey(prov, keyblob.data(), keyblob.length(), 0, CRYPT_IPSEC_HMAC_KEY, &key));
}

CryptoKey::CryptoKey(const CryptoKey& right)
	: key(NULL)
{
	if (right.key) {
		WIN32_BOOLCHECKED(CryptDuplicateKey(right.key, NULL, 0, &key));
	}
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
//	hash_t
//
//////////////////////////////////////////////////////////////////////////////

hash_t::hash_t(HCRYPTPROV prov, ALG_ID hash_alg, bool as_hmac, const CryptoKey& ___key)
	: key(___key)
	, hash(NULL)
{
	WIN32_BOOLCHECKED(CryptCreateHash(prov, as_hmac ? CALG_HMAC : hash_alg, key.data(), 0, &hash));

	if (as_hmac) {
		HMAC_INFO hmacinfo;
		hmacinfo.pbInnerString = NULL;
		hmacinfo.cbInnerString = 0;
		hmacinfo.pbOuterString = NULL;
		hmacinfo.cbOuterString = 0;
		hmacinfo.HashAlgid = hash_alg;

		WIN32_BOOLCHECKED(CryptSetHashParam(hash, HP_HMAC_INFO, (const BYTE*)&hmacinfo, 0));
	}
}

void swap(hash_t& first, hash_t& second) {
	using std::swap;

	swap(first.key, second.key);
	swap(first.hash, second.hash);
}

hash_t::hash_t(hash_t&& old)
	: key(key)
	, hash(NULL)
{
	swap(*this, old);
}

hash_t::hash_t(const hash_t& old)
	: key(key)
	, hash(NULL)
{
	if (old.hash) {
		WIN32_BOOLCHECKED(CryptDuplicateHash(old.hash, NULL, 0, &hash));
	}
}

hash_t& hash_t::operator=(hash_t right) {
	hash_t temp(right);
	swap(*this, temp);
	return (*this);
}

hash_t::~hash_t() {
	CryptDestroyHash(hash);
}

hash_t& hash_t::append(const std::vector<BYTE>& data) {
	WIN32_BOOLCHECKED(CryptHashData(hash, data.data(), data.size(), 0));
	return (*this);
}

std::vector<BYTE> hash_t::finish() {
	DWORD hashlen, hashlensize = sizeof(DWORD);

	WIN32_BOOLCHECKED(CryptGetHashParam(hash, HP_HASHSIZE, (BYTE*)&hashlen, &hashlensize, 0));

	std::vector<BYTE> result(hashlen, 0);

	WIN32_BOOLCHECKED(CryptGetHashParam(hash, HP_HASHVAL, &result[0], &hashlen, 0));
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

void swap(CryptoProvider& left, CryptoProvider& right) {
	using std::swap;

	swap(left.prov, right.prov);	
}

CryptoProvider::CryptoProvider(CryptoProvider&& old)
	: prov(NULL)
{
	swap(*this, old);
}

CryptoProvider::~CryptoProvider() {
	CryptReleaseContext(prov, 0);
}

hash_t CryptoProvider::new_hmac_md5(const std::vector<BYTE>& password) const {
	return hash_t(prov, CALG_MD5, true, CryptoKey(prov, password));
}

hash_t CryptoProvider::new_md4() const {
	return hash_t(prov, CALG_MD4, false, CryptoKey());
}