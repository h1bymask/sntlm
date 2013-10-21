#pragma once
#ifndef _WIN32_SOCKETS_H_
#define _WIN32_SOCKETS_H_

#include "win32message.h"

#include <WinSock2.h>

#include <vector>

class TcpClientSocket {
public:
	TcpClientSocket(const std::string& hostname, USHORT port)
		: s(INVALID_SOCKET)
	{
		s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == s) {
			DWORD error = WSAGetLastError();
			throw win32_exception(error);
		}
	}

	~TcpClientSocket() {
		shutdown(s, SD_BOTH);
		closesocket(s);
	}

	void send(const std::vector<BYTE>& data) {
		size_t tosend = data.size(), allsent = 0;

		while (tosend) { 
			int sent = ::send(s, (const char*)(data.data()) + allsent, tosend, 0);
			if (SOCKET_ERROR == sent) {
				DWORD error = WSAGetLastError();
				throw win32_exception(error);
			}
			tosend -= sent;
			allsent += sent;
		}
	}

	std::vector<BYTE>::iterator recv_upto(std::vector<BYTE>::iterator first, std::vector<BYTE>::iterator last) {
		size_t buffsize = (last - first);

		int len = (buffsize > MAXINT) ? MAXINT : buffsize;
		int recvd = ::recv(s, (char*)(&*first), len, 0);
		if (SOCKET_ERROR == recvd) {
			DWORD error = WSAGetLastError();
			throw win32_exception(error);
		}
		return (first + recvd);
	}

private:
	TcpClientSocket();
	TcpClientSocket(const TcpClientSocket&);
	TcpClientSocket(TcpClientSocket&&);
	TcpClientSocket& operator=(TcpClientSocket);

	SOCKET s;
};

class WS32 {
public:
	WS32() {
		DWORD error = WSAStartup(MAKEWORD(2, 2), &v);
		if (error) {
			throw win32_exception(error);
		}
	}

	~WS32() {
		WSACleanup();
	}

private:
	WS32(const WS32&);
	WS32(WS32&&);
	WS32& operator=(WS32);

	WSADATA v;
};

#endif // _WIN32_SOCKETS_H_