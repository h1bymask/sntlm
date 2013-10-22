#include "http.h"

#include "uconv.h"

#include <algorithm>

http_exception::http_exception(const char* m)
	: std::runtime_error(m)
{ }

http_exception::http_exception(const std::string& m)
	: std::runtime_error(m)
{ }


HttpResponse::HttpResponse(TcpClientSocket& socket)
	: buffer(socket)
{
	std::string requestline = buffer.getline();
	if (requestline.empty()) { throw http_exception("Empty response"); }

	parseResponseLine(requestline);

	std::vector<std::string> raw_headers;
	for (std::string line = buffer.getline(); !line.empty(); line = buffer.getline()) {
		if (('\x20' == line[0]) || ('\t' == line[0])) {
			auto last = raw_headers.rbegin();
			if (raw_headers.rend() == last) { throw http_exception("HTTP request can't start with a whitespace"); }
			if (last->find(':') == std::string::npos) { throw http_exception("Header name can't reside on multiple lines"); }
			*last += line;
		}
		else {
			raw_headers.push_back(line);
		}
	}

	parseHeaders(raw_headers);
}

HttpResponse::~HttpResponse() {
}

void HttpResponse::parseHeaders(const std::vector<std::string>& raw_headers) {
	for (auto it = std::begin(raw_headers), eit = std::end(raw_headers); it != eit; ++it) {
		size_t colon = it->find(':');
		if (std::string::npos == colon) { throw http_exception("Header must have value"); }
		std::string 
			key(it->begin(), it->begin() + colon),
			value(it->begin() + (colon + 1), it->end());

		canonicalizeHeaderName(key);
		trim(value);

		if (value.empty()) { throw http_exception("Header must have value"); }

		if (headers.end() == headers.find(key)) {
			headers[key] = value;
		}
		else {
			headers[key] += ",";
			headers[key] += value;
		}
	}
}

/*
void HttpResponse::parseRequestLine(const std::string& reqline) {
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
*/

void HttpResponse::parseResponseLine(const std::string& reqline) {
	auto reqline_begin = std::begin(reqline), reqline_end = std::end(reqline), 
		sel_begin = reqline_begin, sel_end = sel_begin;

	if (sel_end == reqline_end) { throw http_exception("Empty response"); }
	if ('\x20' == *sel_end) { throw http_exception("Response can't start with whitespace"); }
	while ((sel_end != reqline_end) && ('\x20' != *sel_end)) { ++sel_end; }
	if (sel_end == reqline_end) { throw http_exception("Status code is absent"); }
	version = std::string(sel_begin, sel_end);
	if ((version != "HTTP/1.0") ||(version != "HTTP/1.1")) {
		throw http_exception("Unsupported HTTP version");
	}

	sel_begin = sel_end;
	++sel_begin;
	sel_end = sel_begin;

	if (sel_end == reqline_end) { throw http_exception("Status code is absent"); }
	if ('\x20' == *sel_end) { throw http_exception("Multiple spaces are not allowed in status line"); }
	while ((sel_end != reqline_end) && ('\x20' != *sel_end)) { ++sel_end; }
	if (sel_end == reqline_end) { throw http_exception("Reason phrase is absent"); }
	status = std::string(sel_begin, sel_end);
	
	sel_begin = sel_end;
	++sel_begin;
	sel_end = reqline_end;
	reason = std::string(sel_begin, sel_end);
}

void HttpResponse::canonicalizeHeaderName(std::string& headername) {
	for (auto it = std::begin(headername), eit = std::end(headername); it != eit; it = ++std::find(it, eit, '-')) {
		*it = toupper(*it);
	}
}