/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/25 09:57:41 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 09:58:48 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/HttpResponse.hpp" // Include its own header
#include <iomanip>  // For std::put_time (if C++11, but we use strftime for C++98)
#include <cstdio>   // For snprintf, strftime
#include <vector>   // For std::vector<char>
#include <algorithm> // For std::transform (for toLower in getMimeType if used here)


// --- Helper function implementations (outside the class if generic) ---

// Maps HTTP status codes to their standard messages
std::string getHttpStatusMessage(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found"; // Often used for temporary redirects
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        default: return "Unknown Status"; // Fallback for unhandled codes
    }
}

// Determines the MIME type based on file extension
// This can be expanded significantly. For now, a basic set.
// This is a global helper, so it does not need to be a member of HttpResponse.
// It could also be placed in HttpUtils.cpp if you create one.
std::string getMimeType(const std::string& filePath) {
    size_t dotPos = filePath.rfind('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream"; // Default for no extension
    }
    std::string ext = filePath.substr(dotPos);
    // Convert extension to lowercase for case-insensitive matching
    std::transform(ext.begin(), ext.end(), ext.begin(), static_cast<int(*)(int)>(std::tolower));

    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".txt") return "text/plain";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".xml") return "application/xml";
    // Add more types as needed
    return "application/octet-stream"; // Default fallback
}


// --- HttpResponse Class Implementation ---

// Constructor: Initializes with default HTTP/1.1 protocol and common headers.
HttpResponse::HttpResponse() : _protocolVersion("HTTP/1.1"), _statusCode(200), _statusMessage("OK") {
    setDefaultHeaders();
}

// Destructor
HttpResponse::~HttpResponse() {}

// Sets the HTTP status code and updates the status message accordingly.
void HttpResponse::setStatus(int code) {
    _statusCode = code;
    _statusMessage = getHttpStatusMessage(code);
}

// Adds or updates a header in the response.
// Header names are case-insensitive in HTTP, but typically stored canonical (e.g., "Content-Type").
// This implementation stores them as provided, assuming canonical form is used by caller.
void HttpResponse::addHeader(const std::string& name, const std::string& value) {
    _headers[name] = value;
}

// Sets the response body from a string and updates Content-Length.
void HttpResponse::setBody(const std::string& content) {
    _body.assign(content.begin(), content.end()); // Copy string content to char vector
    // Convert size_t to string for header value
    std::ostringstream oss;
    oss << _body.size();
    addHeader("Content-Length", oss.str());
}

// Sets the response body from a vector of chars (for binary data) and updates Content-Length.
void HttpResponse::setBody(const std::vector<char>& content) {
    _body = content; // Direct copy
    // Convert size_t to string for header value
    std::ostringstream oss;
    oss << _body.size();
    addHeader("Content-Length", oss.str());
}

// Generates the current GMT date/time string for the "Date" header.
// Format: "Day, DD Mon YYYY HH:MM:SS GMT" (RFC 1123)
std::string HttpResponse::getCurrentGmTime() const {
    char buf[100];
    time_t rawtime;
    struct tm *gmtm;

    time(&rawtime);
    gmtm = gmtime(&rawtime); // Get GMT time
    
    // Format according to RFC 1123: "Wdy, DD Mon YYYY HH:MM:SS GMT"
    // strftime returns the number of characters placed into the array pointed to by buf
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtm);
    return std::string(buf);
}

// Sets common default headers that should typically be present in a response.
void HttpResponse::setDefaultHeaders() {
    addHeader("Server", "Webserv/1.0"); // Identify your server
    addHeader("Date", getCurrentGmTime()); // Current date and time
    // addHeader("Connection", "keep-alive"); // Often implied by HTTP/1.1, but can be explicit
}

// Generates the complete raw HTTP response string.
std::string HttpResponse::toString() const {
    std::ostringstream oss;

    // 1. Status Line
    oss << _protocolVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

    // 2. Headers
    // Ensure essential headers are present/updated before sending
    // For Content-Length, it's set by setBody methods.
    // For Content-Type, it should be set by the handler.

    // If Content-Type is not set by handler, provide a default
    if (_headers.find("Content-Type") == _headers.end()) {
        oss << "Content-Type: application/octet-stream\r\n"; // Default if not specified
    }

    // Write all collected headers
    std::map<std::string, std::string>::const_iterator it;
    for (it = _headers.begin(); it != _headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }
    
    oss << "\r\n"; // End of headers

    // 3. Body
    // Append body content from the vector<char>
    for (size_t i = 0; i < _body.size(); ++i) {
        oss << _body[i];
    }

    return oss.str();
}
