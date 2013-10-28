#pragma once
#ifndef _NTLM_H_
#define _NTLM_H_

#include "win32crypto.h"

std::vector<BYTE> NTLMOWFv2(const CryptoProvider& provider, const std::wstring& password, const std::wstring& user, const std::wstring& domain);

class ntlm_request_t {
public:
	ntlm_request_t(const std::string& domain, const std::string& workstation);
	~ntlm_request_t();

	friend std::ostream& operator<<(std::ostream& s, const ntlm_request_t& value);
private:
	ntlm_request_t();
	ntlm_request_t(const ntlm_request_t&);
	ntlm_request_t(ntlm_request_t&&);
	ntlm_request_t& operator=(ntlm_request_t);

	std::string domain, workstation;
};

//std::ostream& operator<<(std::ostream& s, const ntlm_request_t& value);

#endif // _NTLM_H_