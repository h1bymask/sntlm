#include "win32sockets.h"

#include "win32message.h"
#include "numconv.h"
#include <WS2tcpip.h>
#include <algorithm>

//////////////////////////////////////////////////////////////////////////////
//
//  TcpClientSocket
//
//////////////////////////////////////////////////////////////////////////////

TcpClientSocket::TcpClientSocket(const std::string& hostname, USHORT port)
	: s(INVALID_SOCKET)
{
	s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == s) {
		DWORD error = WSAGetLastError();
		throw win32_exception(error);
	}

	addrinfo hint, *result;
	ZeroMemory(&hint, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;
	if (GetAddrInfoA(hostname.c_str(), numtostr(port).c_str(), &hint, &result)) {
		DWORD error = WSAGetLastError();
		throw win32_exception(error);
	}

	std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> resultholder(result, ::freeaddrinfo);

	sockaddr_in addr = *(sockaddr_in*)result->ai_addr;

	if (SOCKET_ERROR == ::connect(s, (sockaddr*)&addr, sizeof(sockaddr_in))) {
		DWORD error = WSAGetLastError();
		throw win32_exception(error);
	}
}

void swap(TcpClientSocket& left, TcpClientSocket& right) {
	using std::swap;

	swap(left.s, right.s);
}

TcpClientSocket::TcpClientSocket(TcpClientSocket&& old)
	: s(INVALID_SOCKET)
{
	swap(*this, old);
}

TcpClientSocket::~TcpClientSocket() {
	::shutdown(s, SD_BOTH);
	::closesocket(s);
}

//void TcpClientSocket::send(std::vector<BYTE>::const_iterator first, std::vector<BYTE>::const_iterator last) {
//	while (first != last) { 
//		int sent = ::send(s, (const char*)(&*first), last - first, 0);
//		if (SOCKET_ERROR == sent) {
//			DWORD error = WSAGetLastError();
//			throw win32_exception(error);
//		}
//		first += sent;
//	}
//}

//std::vector<BYTE>::iterator TcpClientSocket::recv_upto(std::vector<BYTE>::iterator first, std::vector<BYTE>::iterator last) {
//	size_t buffsize = (last - first);
//
//	int len = (buffsize > MAXINT) ? MAXINT : buffsize;
//	int recvd = ::recv(s, (char*)(&*first), len, 0);
//	if (SOCKET_ERROR == recvd) {
//		DWORD error = WSAGetLastError();
//		throw win32_exception(error);
//	}
//	return (first + recvd);
//}


//////////////////////////////////////////////////////////////////////////////
//
//  WS32
//
//////////////////////////////////////////////////////////////////////////////

WS32::WS32() {
	DWORD error = WSAStartup(MAKEWORD(2, 2), &v);
	if (error) {
		throw win32_exception(error);
	}
}

WS32::~WS32() {
	WSACleanup();
}


//////////////////////////////////////////////////////////////////////////////
//
//  SocketBuffer
//
//////////////////////////////////////////////////////////////////////////////

SocketBuffer::SocketBuffer(TcpClientSocket& socket) 
	: socket(socket)
	, buffer(8 * 1024, 0)
	, last(std::begin(buffer))
{ }

SocketBuffer::~SocketBuffer() { }

std::string SocketBuffer::getline() {
	std::string result;

	auto crpos = std::find(std::begin(buffer), last, '\r');
	if (crpos == last) {
		// No CR were in the buffer
		result.append(std::begin(buffer), last);

		last = socket.recv_upto(std::begin(buffer), std::end(buffer));
		if (std::begin(buffer) == last) {
			// EOF was reached before CRLF
			throw std::runtime_error("Premature EOF");
		}

		result += getline();
		return result;
	}
	else {
		// We've seen a CR. Is there an LF right after it?
		auto lfpos = crpos;
		++lfpos;

		if (std::end(buffer) == lfpos) {
			// Well, we have to recv to find out! But let's not remove the CR from the buffer
			result.append(std::begin(buffer), crpos);
			last = std::copy(crpos, last, std::begin(buffer));

			last = socket.recv_upto(last, std::end(buffer));
			result += getline();
			return result;
		}
		if ('\n' == *lfpos) {
			// Yes, CRLF combination found
			result.append(std::begin(buffer), crpos);

			// Now we remove the line and CRLF from the buffer
			++lfpos;
			last = std::copy(lfpos, last, std::begin(buffer));

			// That's the final result
			return result;
		}
		else {
			// Nah, just a lonely CR. Include it in
			result.append(std::begin(buffer), lfpos);

			// Now remove from the buffer what we included and move on
			last = std::copy(lfpos, last, std::begin(buffer));

			result += getline();
			return result;
		}
	}
}

std::string SocketBuffer::getbuffer() const {
	return std::string(std::begin(buffer), std::vector<BYTE>::const_iterator(last));
}

std::string SocketBuffer::getcount(size_t byte_count) {
	std::string result;
	if (!byte_count) { return result; }

	size_t buffpresent = last - std::begin(buffer);

	if (byte_count <= buffpresent) {
		result = std::string(std::begin(buffer), std::begin(buffer) + byte_count);
		last = std::copy(std::begin(buffer) + byte_count, last, std::begin(buffer));
		return result;
	}
	else {
		result = getbuffer();
		last = socket.recv_upto(std::begin(buffer), std::end(buffer));
		if (std::begin(buffer) == last) { throw std::runtime_error("Premature EOF"); }
		return result + getcount(byte_count - buffpresent);
	}
}

std::string SocketBuffer::getchunk() {
	std::string chunksize_raw = getline();
	DWORD chunksize = 0;
	if (strtonum(chunksize_raw, chunksize)) {
		if (chunksize) {
			std::string result = getcount(chunksize);
			if (!getline().empty()) { throw std::runtime_error("CRLF after chunk-data was expected"); }
			return result;
		}
		else {
			return "";
		}
	}
	else {
		throw std::runtime_error("Invalid chunk-size");
	}
}
