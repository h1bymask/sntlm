#pragma once
#ifndef _WIN32_SOCKETS_H_
#define _WIN32_SOCKETS_H_

#include <WinSock2.h>
#include <vector>
#include "numconv.h"

class TcpClientSocket {
public:
	TcpClientSocket(const std::string& hostname, USHORT port);
	friend void swap(TcpClientSocket& left, TcpClientSocket& right);
	TcpClientSocket(TcpClientSocket&& old);
	~TcpClientSocket();

	template <typename Iter>
	typename std::enable_if<is_char<typename Iter::value_type>::value, void>::type
	send(Iter first, Iter last) {
		while (first != last) { 
			int sent = ::send(s, reinterpret_cast<const char*>(&*first), last - first, 0);
			if (SOCKET_ERROR == sent) {
				DWORD error = WSAGetLastError();
				throw win32_exception(error);
			}
			first += sent;
		}
	}

	template <typename Iter>
	typename std::enable_if<is_char<typename Iter::value_type>::value, Iter>::type
	recv_upto(Iter first, Iter last) {
		size_t buffsize = (last - first);

		int len = (buffsize > MAXINT) ? MAXINT : buffsize;
		int recvd = ::recv(s, reinterpret_cast<char*>(&*first), len, 0);
		if (SOCKET_ERROR == recvd) {
			DWORD error = WSAGetLastError();
			throw win32_exception(error);
		}
		return (first + recvd);
	}

private:
	TcpClientSocket();
	TcpClientSocket(const TcpClientSocket&);
	TcpClientSocket& operator=(TcpClientSocket);

	SOCKET s;
};

class WS32 {
public:
	WS32();
	~WS32();

private:
	WS32(const WS32&);
	WS32(WS32&&);
	WS32& operator=(WS32);

	WSADATA v;
};

class SocketBuffer {
public:
	SocketBuffer(TcpClientSocket& socket);
	~SocketBuffer();

	std::string getline();
	std::string getcount(size_t byte_count);
	std::string getchunk();
	std::string getbuffer() const;
private:
	SocketBuffer();
	SocketBuffer(const SocketBuffer&);
	SocketBuffer(SocketBuffer&&);
	SocketBuffer& operator=(SocketBuffer);

	TcpClientSocket& socket;
	std::vector<BYTE> buffer;
	std::vector<BYTE>::iterator last;
};

#endif // _WIN32_SOCKETS_H_