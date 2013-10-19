#pragma once
#ifndef _NTLM_H_
#define _NTLM_H_

#include "win32crypto.h"

std::vector<BYTE> NTOWFv2(const CryptoProvider& provider, std::wstring& password, const std::wstring& user, const std::wstring& domain);
std::vector<BYTE> LMOWFv2(const CryptoProvider& provider, std::wstring& password, const std::wstring& user, const std::wstring& domain);

#endif // _NTLM_H_