#pragma once
#ifndef _WIN32_SOCKETS_H_
#define _WIN32_SOCKETS_H_

#include <WinSock2.h>
#include <vector>

class TcpClientSocket {
public:
	TcpClientSocket(const std::string& hostname, USHORT port);
	friend void swap(TcpClientSocket& left, TcpClientSocket& right);
	TcpClientSocket(TcpClientSocket&& old);
	~TcpClientSocket();

	void send(std::vector<BYTE>::const_iterator first, std::vector<BYTE>::const_iterator last);
	std::vector<BYTE>::iterator recv_upto(std::vector<BYTE>::iterator first, std::vector<BYTE>::iterator last);

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