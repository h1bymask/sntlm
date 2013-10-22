#include "http.h"

#include "uniconv.h"
#include "numconv.h"

#include <algorithm>

http_exception::http_exception(const char* m)
	: std::runtime_error(m)
{ }

http_exception::http_exception(const std::string& m)
	: std::runtime_error(m)
{ }


HttpResponse::HttpResponse(TcpClientSocket& socket)
	: buffer(socket)
	, ischunked(false)
	, contentlength(nlen)
{
	raw_statusline = buffer.getline();
	if (raw_statusline.empty()) { throw http_exception("Empty response"); }

	parseResponseLine(raw_statusline);

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

const HttpResponse::headers_t& HttpResponse::getHeaders() const { return headers; }
const std::string& HttpResponse::getStatusLine() const { return raw_statusline; }
HttpResponse::status_code HttpResponse::getStatusCode() const { return status; }
SocketBuffer& HttpResponse::getBuffer() { return buffer; }
bool HttpResponse::getIsChunked() const { return ischunked; }
size_t HttpResponse::getContentLength() const { return contentlength; }


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

		auto& d = headers[key];
		d.insert(std::end(d), value);
	}

	auto transfer_encoding = headers.find("Transfer-Encoding");
	if (std::end(headers) != transfer_encoding) {
		std::string lastencoding = *transfer_encoding->second.rbegin();
		toLower(lastencoding, CP_ISO8859_1);
		if ("chunked" != lastencoding) { throw http_exception("\"chunked\" must be the last transfer encoding applied to the message"); }
		ischunked = true;
		return;
	}

	// "Transfer-Encoding: chunked" is not present
	auto content_length = headers.find("Content-Length");
	if (std::end(headers) != content_length) {
		if (content_length->second.size() > 1) { throw http_exception("Multiple Content-Length headers are not allowed"); }
		
		size_t temp;
		if (!strtonum(*content_length->second.begin(), temp) || nlen == temp) { throw http_exception("Invalid Content-Length"); }
		contentlength = temp;
		return;
	}

	// Neither "Transfer-Endcoding: chunked" nor valid Content-Length are present
	auto connection = headers.find("Connection");
	if (std::end(headers) != connection) {	
		for (auto it = std::begin(connection->second), eit = std::end(connection->second); it != eit; ++it) {
			std::string temp = *it;
			toLower(temp, CP_ISO8859_1);
			if (temp == "close") { return; }
		}
	}

	// Neither "Transfer-Endcoding: chunked", valid Content-Length, nor "Connection: close" are present -- that's malformed HTTP/1.1 response
	throw http_exception("Can't obtain message length");	
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
	if ((version != "HTTP/1.0") && (version != "HTTP/1.1")) {
		throw http_exception("Unsupported HTTP version");
	}

	sel_begin = sel_end;
	++sel_begin;
	sel_end = sel_begin;

	if (sel_end == reqline_end) { throw http_exception("Status code is absent"); }
	if ('\x20' == *sel_end) { throw http_exception("Multiple spaces are not allowed in status line"); }
	while ((sel_end != reqline_end) && ('\x20' != *sel_end)) { ++sel_end; }
	if (sel_end == reqline_end) { throw http_exception("Reason phrase is absent"); }
	std::string status_raw = std::string(sel_begin, sel_end);
	if (!strtonum(status_raw, status) &&  ((status < 100) || (status > 999))) {
		throw http_exception("Invalid status"); 
	}
	
	sel_begin = sel_end;
	++sel_begin;
	sel_end = reqline_end;
	reason = std::string(sel_begin, sel_end);
}

void HttpResponse::canonicalizeHeaderName(std::string& headername) {
	for (size_t i = 0; i < headername.length(); ) {
		headername[i] = toupper(headername[i]);

		i = headername.find('-', i);
		i = (std::string::npos == i) ? headername.length() : i + 1;
	}
}