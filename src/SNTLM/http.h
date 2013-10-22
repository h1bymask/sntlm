#pragma once
#ifndef _HTTP_H_
#define _HTTP_H_

#include "win32sockets.h"

#include <map>
#include <string>

class http_exception : public std::runtime_error {
public:
	explicit http_exception(const char* m);
	explicit http_exception(const std::string& m);
};

class HttpResponse {
public:
	typedef USHORT status_code;

	static const status_code StatusOK					= 200;
	static const status_code StatusBadRequest			= 400;
	static const status_code StatusProxyAuthRequired	= 407;
	static const status_code StatusVersionNotSupported	= 505;

	HttpResponse(TcpClientSocket& socket);		
	~HttpResponse();

	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getStatusLine() const;
	status_code getStatusCode() const;
	SocketBuffer& getBuffer();

	static const size_t nlen = (size_t)(-1);
	size_t getContentLength() const;
private:
	HttpResponse(const HttpResponse&);
	HttpResponse(HttpResponse&&);
	HttpResponse& operator=(HttpResponse);

	void parseHeaders(const std::vector<std::string>& raw_headers);
	void parseResponseLine(const std::string& reqline);
	
	void canonicalizeHeaderName(std::string& headername);

	SocketBuffer buffer;
	std::string raw_statusline, version;
	status_code status;
	std::string reason;
	std::map<std::string, std::string> headers;
};

#endif //_HTTP_H_