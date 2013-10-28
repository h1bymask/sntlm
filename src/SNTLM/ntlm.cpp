#include "ntlm.h"
#include "uniconv.h"

std::vector<BYTE> NTLMOWFv2(const CryptoProvider& provider, const std::wstring& password, const std::wstring& user, const std::wstring& domain) {
	hash_t hmac_md5 = provider.new_hmac_md5(
		provider.new_md4().append(dump_memory(password)).finish()
	);

	std::wstring upcase_user(user), upcase_domain(domain);
	toUpper(upcase_user), toUpper(upcase_domain);

	return hmac_md5
		.append(dump_memory(upcase_user))
		.append(dump_memory(upcase_domain))
		.finish();
}


ntlm_request_t::ntlm_request_t(const std::string& domain, const std::string& workstation)
	: domain(domain)
	, workstation(workstation)
{
	toUpper(this->domain);
	toUpper(this->workstation);
}

ntlm_request_t::~ntlm_request_t() {
}

struct U32LE {
public:
	U32LE(DWORD x) : x(x) { }
	~U32LE() { }
	
	friend std::ostream& operator<<(std::ostream& s, U32LE value);
private:
	DWORD x;
};

struct U16LE {
public:
	U16LE(WORD x) : x(x) { }
	~U16LE() { }
	
	friend std::ostream& operator<<(std::ostream& s, U16LE value);
private:
	DWORD x;
};


std::ostream& operator<<(std::ostream& s, U32LE value) {
	return
	s	<< LOBYTE(LOWORD(value.x)) << HIBYTE(LOWORD(value.x))
		<< LOBYTE(HIWORD(value.x)) << HIBYTE(HIWORD(value.x));
}

std::ostream& operator<<(std::ostream& s, U16LE value) {
	return s << LOBYTE(LOWORD(value.x)) << HIBYTE(LOWORD(value.x));
}

std::ostream& operator<<(std::ostream& s, const ntlm_request_t& value) {
	size_t dlen = value.domain.length(), hlen = value.workstation.length();
	if ((dlen > MAXWORD) || (hlen > MAXWORD)) {
		throw std::invalid_argument("Domain and/or workstation name is too long");
	}

	return
	s	<< "NTLMSSP" << '\0' << U32LE(1UL) << U32LE(0xA208B205UL)  // this number is magical
		<< U16LE(dlen) << U16LE(dlen) << U32LE(32 + hlen)
		<< U16LE(hlen) << U16LE(hlen) << U32LE(32)
		<< value.workstation << value.domain;
}


//void ntlm2_calc_resp(char **nthash, int *ntlen, char **lmhash, int *lmlen, char *passnt2, char *challenge, int tbofs, int tblen) {
//	char *tmp, *blob, *nonce, *buf;
//	int64_t tw;
//	int blen;
//
//	nonce = new(8 + 1);
//	VAL(nonce, uint64_t, 0) = ((uint64_t)random() << 32) | random();
//	tw = ((uint64_t)time(NULL) + 11644473600LLU) * 10000000LLU;
//
//	blob = new(4+4+8+8+4+tblen+4 + 1);
//	VAL(blob, uint32_t, 0) = U32LE(0x00000101);
//	VAL(blob, uint32_t, 4) = U32LE(0);
//	VAL(blob, uint64_t, 8) = U64LE(tw);
//	VAL(blob, uint64_t, 16) = U64LE(VAL(nonce, uint64_t, 0));
//	VAL(blob, uint32_t, 24) = U32LE(0);
//	memcpy(blob+28, MEM(challenge, char, tbofs), tblen);
//	VAL(blob, uint32_t, 28+tblen) = U32LE(0);
//	blen = 28+tblen+4;
//
//	*ntlen = 16+blen;
//	*nthash = new(*ntlen + 1);
//	buf = new(8+blen + 1);
//	memcpy(buf, MEM(challenge, char, 24), 8);
//	memcpy(buf+8, blob, blen);
//	hmac_md5(passnt2, 16, buf, 8+blen, *nthash);
//	memcpy(*nthash+16, blob, blen);
//	free(buf);
//
//	*lmlen = 24;
//	*lmhash = new(*lmlen + 1);
//	buf = new(16 + 1);
//	memcpy(buf, MEM(challenge, char, 24), 8);
//	memcpy(buf+8, nonce, 8);
//	hmac_md5(passnt2, 16, buf, 16, *lmhash);
//	memcpy(*lmhash+16, nonce, 8);
//	free(buf);
//
//	free(blob);
//	free(nonce);
//	return;
//}
//
//
//int ntlm_request(char **dst, struct auth_s *creds) {
//	char *buf, *tmp;
//	int dlen, hlen;
//	uint32_t flags = 0xa208b205;
//
//	*dst = NULL;
//	dlen = strlen(creds->domain);
//	hlen = strlen(creds->workstation);
//
//	buf = new(NTLM_BUFSIZE);
//	memcpy(buf, "NTLMSSP\0", 8);
//	VAL(buf, uint32_t, 8) = U32LE(1);
//	VAL(buf, uint32_t, 12) = U32LE(flags);
//	VAL(buf, uint16_t, 16) = U16LE(dlen);
//	VAL(buf, uint16_t, 18) = U16LE(dlen);
//	VAL(buf, uint32_t, 20) = U32LE(32 + hlen);
//	VAL(buf, uint16_t, 24) = U16LE(hlen);
//	VAL(buf, uint16_t, 26) = U16LE(hlen);
//	VAL(buf, uint32_t, 28) = U32LE(32);
//
//	tmp = uppercase(strdup(creds->workstation));
//	memcpy(buf+32, tmp, hlen);
//	free(tmp);
//
//	tmp = uppercase(strdup(creds->domain));
//	memcpy(buf+32+hlen, tmp, dlen);
//	free(tmp);
//
//	*dst = buf;
//	return 32+dlen+hlen;
//}
//
//
//int ntlm_response(char **dst, char *challenge, int challen, struct auth_s *creds) {
//	char *buf, *udomain, *uuser, *uhost, *tmp;
//	int dlen, ulen, hlen;
//	uint16_t tpos, tlen, ttype = -1, tbofs = 0, tblen = 0;
//	char *lmhash = NULL, *nthash = NULL;
//	int lmlen = 0, ntlen = 0;
//
//	if (challen > 48) {
//		tbofs = tpos = U16LE(VAL(challenge, uint16_t, 44));
//		while (tpos+4 <= challen && (ttype = U16LE(VAL(challenge, uint16_t, tpos)))) {
//			tlen = U16LE(VAL(challenge, uint16_t, tpos+2));
//			if (tpos+4+tlen > challen)
//				break;
//
//			if (debug) {
//				switch (ttype) {
//					case 0x1:
//						printf("\t   Server: ");
//						break;
//					case 0x2:
//						printf("\tNT domain: ");
//						break;
//					case 0x3:
//						printf("\t     FQDN: ");
//						break;
//					case 0x4:
//						printf("\t   Domain: ");
//						break;
//					case 0x5:
//						printf("\t      TLD: ");
//						break;
//					default:
//						printf("\t      %3d: ", ttype);
//						break;
//				}
//				tmp = printuc(MEM(challenge, char, tpos+4), tlen);
//				printf("%s\n", tmp);
//				free(tmp);
//			}
//
//			tpos += 4+tlen;
//			tblen += 4+tlen;
//		}
//
//		if (tblen && ttype == 0)
//			tblen += 4;
//
//		if (debug) {
//			printf("\t    TBofs: %d\n\t    TBlen: %d\n\t    ttype: %d\n", tbofs, tblen, ttype);
//		}
//	}
//
//	if (!tblen) {
//		return 0;
//	}
//
//	ntlm2_calc_resp(&nthash, &ntlen, &lmhash, &lmlen, creds->passntlm2, challenge, tbofs, tblen);
//
//	tmp = uppercase(strdup(creds->domain));
//	dlen = unicode(&udomain, tmp);
//	free(tmp);
//	ulen = unicode(&uuser, creds->user);
//	tmp = uppercase(strdup(creds->workstation));
//	hlen = unicode(&uhost, tmp);
//	free(tmp);
//
//	buf = new(NTLM_BUFSIZE);
//	memcpy(buf, "NTLMSSP\0", 8);
//	VAL(buf, uint32_t, 8) = U32LE(3);
//
//	/* LM */
//	VAL(buf, uint16_t, 12) = U16LE(lmlen);
//	VAL(buf, uint16_t, 14) = U16LE(lmlen);
//	VAL(buf, uint32_t, 16) = U32LE(64+dlen+ulen+hlen);
//
//	/* NT */
//	VAL(buf, uint16_t, 20) = U16LE(ntlen);
//	VAL(buf, uint16_t, 22) = U16LE(ntlen);
//	VAL(buf, uint32_t, 24) = U32LE(64+dlen+ulen+hlen+lmlen);
//
//	/* Domain */
//	VAL(buf, uint16_t, 28) = U16LE(dlen);
//	VAL(buf, uint16_t, 30) = U16LE(dlen);
//	VAL(buf, uint32_t, 32) = U32LE(64);
//
//	/* Username */
//	VAL(buf, uint16_t, 36) = U16LE(ulen);
//	VAL(buf, uint16_t, 38) = U16LE(ulen);
//	VAL(buf, uint32_t, 40) = U32LE(64+dlen);
//
//	/* Hostname */
//	VAL(buf, uint16_t, 44) = U16LE(hlen);
//	VAL(buf, uint16_t, 46) = U16LE(hlen);
//	VAL(buf, uint32_t, 48) = U32LE(64+dlen+ulen);
//
//	/* Session */
//	VAL(buf, uint16_t, 52) = U16LE(0);
//	VAL(buf, uint16_t, 54) = U16LE(0);
//	VAL(buf, uint16_t, 56) = U16LE(64+dlen+ulen+hlen+lmlen+ntlen);
//
//	/* Flags */
//	VAL(buf, uint32_t, 60) = VAL(challenge, uint32_t, 20);
//
//	memcpy(MEM(buf, char, 64), udomain, dlen);
//	memcpy(MEM(buf, char, 64+dlen), uuser, ulen);
//	memcpy(MEM(buf, char, 64+dlen+ulen), uhost, hlen);
//	memcpy(MEM(buf, char, 64+dlen+ulen+hlen), lmhash, lmlen);
//	memcpy(MEM(buf, char, 64+dlen+ulen+hlen+24), nthash, ntlen);
//
//	free(uhost);
//	free(uuser);
//	free(udomain);
//
//	*dst = buf;
//	return 64+dlen+ulen+hlen+lmlen+ntlen;
//}
