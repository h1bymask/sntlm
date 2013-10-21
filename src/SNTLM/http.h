#pragma once
#ifndef _HTTP_H_
#define _HTTP_H_

#include "win32sockets.h"

#include <map>
#include <string>

#include <algorithm>

	bool ishtsp(char x) {
		return ('\x20' == x) || ('\t' == x);
	}

class http_exception : public std::runtime_error {
public:
	typedef USHORT status_code;

	static const status_code StatusOK					= 200;
	static const status_code StatusBadRequest			= 400;
	static const status_code StatusVersionNotSupported	= 505;

	explicit http_exception(const char* m, status_code c)
		: std::runtime_error(m)
		, statuscode(c)
	{ }

	explicit http_exception(const std::string& m, status_code c)
		: std::runtime_error(m)
		, statuscode(c)
	{ }

	status_code getStatusCode() const {	return statuscode; }

private:
	status_code statuscode;
};

class HttpResponse {
public:
	HttpResponse(TcpClientSocket& socket)
		: buffer(socket)
	{
		std::string requestline = buffer.getline();
		if (requestline.empty()) { throw http_exception("Empty request", http_exception::StatusBadRequest); }

		parseRequestLine(requestline);

		std::vector<std::string> raw_headers;
		for (std::string line = buffer.getline(); !line.empty(); line = buffer.getline()) {
			if (('\x20' == line[0]) || ('\t' == line[0])) {
				auto last = raw_headers.rbegin();
				if (raw_headers.rend() == last) { throw http_exception("HTTP request can't start with a whitespace", http_exception::StatusBadRequest); }
				if (last->find(':') == std::string::npos) { throw http_exception("Header name can't reside on multiple lines", http_exception::StatusBadRequest); }
				*last += line;
			}
			else {
				raw_headers.push_back(line);
			}
		}

		parseHeaders(raw_headers);
	}
		
	~HttpResponse();

private:
	HttpResponse(const HttpResponse&);
	HttpResponse(HttpResponse&&);
	HttpResponse& operator=(HttpResponse);

	void parseHeaders(const std::vector<std::string>& raw_headers) {
		for (auto it = std::begin(raw_headers), eit = std::end(raw_headers); it != eit; ++it) {
			size_t colon = it->find(':');
			if (std::string::npos == colon) { throw http_exception("Header must have value", http_exception::StatusBadRequest); }
			std::string 
				key(it->begin(), it->begin() + colon),
				value(it->begin() + (colon + 1), it->end());

			canonicalizeHeaderName(key);
			trim(value);

			if (value.empty()) { throw http_exception("Header must have value", http_exception::StatusBadRequest); }

			if (headers.end() == headers.find(key)) {
				headers[key] = value;
			}
			else {
				headers[key] += ",";
				headers[key] += value;
			}
		}
	}

	void parseRequestLine(const std::string& reqline) {
		auto reqline_begin = std::begin(reqline), reqline_end = std::end(reqline);

		std::string::const_iterator
			method_begin = reqline_begin, 
			method_end = std::find_if(method_begin, reqline_end, ishtsp);
		if (method_begin == method_end) { throw http_exception("Request can't start with whitespace", http_exception::StatusBadRequest); }
		if (method_end == reqline_end) { throw http_exception("RequestURI is absent", http_exception::StatusBadRequest); }
		method = std::string(reqline_begin, method_end);

		std::string::const_iterator
			url_begin = std::find_if_not(method_end, reqline_end, ishtsp),
			url_end = std::find_if(url_begin, reqline_end, ishtsp);
		if (url_end == reqline_end) { throw http_exception("HTTP version is absent", http_exception::StatusBadRequest); }
		url = std::string(url_begin, url_end);

		std::string::const_iterator
			version_begin = std::find_if_not(url_end, reqline_end, ishtsp),
			version_end = std::find_if(version_begin, reqline_end, ishtsp);
		if (version_end != reqline_end) { throw http_exception("Request line must end with HTTP version", http_exception::StatusBadRequest); }
		version = std::string(version_begin, version_end);

		std::string http_ver = "HTTP/";
		if (version.substr(0, http_ver.length()) == http_ver) {
			throw http_exception("Invalid HTTP version", http_exception::StatusBadRequest);
		}

		if (version != "HTTP/1.1") {
			throw http_exception("Unsupported HTTP version", http_exception::StatusVersionNotSupported);
		}
	}

	void trim(std::string& s) {
		s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), ishtsp));
		s.erase(std::find_if_not(s.rbegin(), s.rend(), ishtsp).base(), s.end());
	}

	void canonicalizeHeaderName(std::string& headername) {
		for (auto it = std::begin(headername), eit = std::end(headername); it != eit; it = ++std::find(it, eit, '-')) {
			*it = toupper(*it);
		}
	}

	SocketBuffer buffer;
	std::string method, url, version;
	std::map<std::string, std::string> headers;
};

#endif //_HTTP_H_