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
		send_impl(reinterpret_cast<const char*>(&*first), last - first);
	}

	template <typename Iter>
	typename std::enable_if<is_char<typename Iter::value_type>::value, Iter>::type
	recv_upto(Iter first, Iter last) {
		return (first + recv_upto_impl(reinterpret_cast<char*>(&*first), last - first));
	}

private:
	TcpClientSocket();
	TcpClientSocket(const TcpClientSocket&);
	TcpClientSocket& operator=(TcpClientSocket);

	void send_impl(const char* data, size_t len);
	size_t recv_upto_impl(char *buffer, size_t buffsize);

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