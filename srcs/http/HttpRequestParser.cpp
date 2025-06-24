/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequestParser.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/23 15:27:03 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/24 13:19:07 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/HttpRequestParser.hpp"
#include "../../includes/utils/StringUtils.hpp" // For trim, isDigits, stringToLong

#include <iostream>
#include <sstream> // For std::istringstream
#include <cctype>  // For std::tolower, std::isspace (used in StringUtils if not already there)

// Implementation of httpMethodToString (for debugging/printing, optional but good practice)
std::string httpMethodToString(HttpMethod method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_DELETE: return "DELETE";
        case HTTP_UNKNOWN: return "UNKNOWN";
        default: return "INVALID_ENUM_VALUE";
    }
}

// Default constructor
HttpRequestParser::HttpRequestParser() : _request() {
    _request.currentState = HttpRequest::RECV_REQUEST_LINE;
}

// Destructor
HttpRequestParser::~HttpRequestParser() {
}

// Appends new raw data to the internal buffer
void HttpRequestParser::appendData(const char* data, size_t len) {
    if (data && len > 0) {
        _buffer.insert(_buffer.end(), data, data + len);
    }
}

// Helper to find a substring (pattern) within a std::vector<char> same as std::string::find but for vectors
size_t HttpRequestParser::findInVector(const std::string& pattern) {
    if (_buffer.empty() || pattern.empty() || pattern.length() > _buffer.size()) {
        return std::string::npos;
    }
    for (size_t i = 0; i <= _buffer.size() - pattern.length(); ++i) {
        bool match = true;
        for (size_t j = 0; j < pattern.length(); ++j) {
            if (_buffer[i+j] != pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return i;
        }
    }
    return std::string::npos;
}

// remove parsed data from the beginning of _buffer that have already been parsed and processed
void HttpRequestParser::consumeBuffer(size_t count) {
    if (count > _buffer.size()) { // guard against count > buffer : should never happen
        _buffer.clear();
    } else {
        _buffer.erase(_buffer.begin(), _buffer.begin() + count); // normal way to remove the beginning of _buffer
    }
}

// set the error state and potentially log a message
void HttpRequestParser::setError(const std::string& msg) {
    _request.currentState = HttpRequest::ERROR;
    std::cerr << "HTTP Parsing Error: " << msg << std::endl;
}

// parsing the request line, first of 3-part request (request line, headers, body)
void HttpRequestParser::parseRequestLine() {
    // 1 - find the end of the request line : reached at the first crlf
    size_t crlf_pos = findInVector(CRLF);

    if (crlf_pos == std::string::npos) { 
        return; // not enough data yet, wait for more
    }

    // 2 - extract the request line up to crlf_pos
    std::string requestLine( _buffer.begin(), _buffer.begin() + crlf_pos );
    
    // 3 - parse the method : should be the first word
    size_t first_space = requestLine.find(' ');
    if (first_space == std::string::npos) {
        setError("Malformed request line: Missing method or URI.");
        return;
    }
    _request.method = requestLine.substr(0, first_space); // Store raw string method

    // 4 - parse the URI : should be the second word
    size_t second_space = requestLine.find(' ', first_space + 1);
    if (second_space == std::string::npos) {
        setError("Malformed request line: Missing URI or protocol version.");
        return;
    }
    _request.uri = requestLine.substr(first_space + 1, second_space - (first_space + 1));

    // 5 - parse the protocol version : should be the last word
    _request.protocolVersion = requestLine.substr(second_space + 1);

    // 6 - basic validation
    if (_request.method.empty() || _request.uri.empty() || _request.protocolVersion.empty()) {
        setError("Malformed request line: Empty component.");
        return;
    }
    if (_request.protocolVersion != "HTTP/1.1") {
        setError("Unsupported protocol version. Only HTTP/1.1 is supported.");
        return;
    }
    // Note: Validation if method is GET/POST/DELETE occurs in later logic.
    // Here, we just store the string and ensure basic format.

    // 7 - consume and advance
    consumeBuffer(crlf_pos + CRLF.length());

    // Decompose URI here, as path and query parameters are known after the request line
    decomposeURI(); 

    _request.currentState = HttpRequest::RECV_HEADERS;
}

// parsing headers, second of 3-part request
void HttpRequestParser::parseHeaders() {
    // FIX: Handle the special case where headers are empty.
    // If the buffer contains exactly "\r\n" after the request line, it means the entire
    // request was "Request-Line\r\n\r\n", signifying no headers and end of request.
    if (_buffer.size() == CRLF.length() && _buffer[0] == '\r' && _buffer[1] == '\n') {
        consumeBuffer(CRLF.length()); // Consume the final CRLF that signifies empty headers
        _request.currentState = HttpRequest::COMPLETE;
        // No extraneous data check needed here, as we specifically matched exact content.
        return;
    }
    
    // Normal header parsing: Look for the DOUBLE_CRLF (end of header block)
    size_t double_crlf_pos = findInVector(DOUBLE_CRLF);

    if (double_crlf_pos == std::string::npos) {
        return; // Not enough data for the full headers block, wait for more.
    }

    // Extract and parse actual headers (if any)
    std::string allHeaders( _buffer.begin(), _buffer.begin() + double_crlf_pos );
    std::istringstream iss(allHeaders);
    std::string line;

    while (std::getline(iss, line, '\r')) {
        if (!line.empty() && line[0] == '\n') {
            line = line.substr(1);
        }
        if (line.empty()) continue; // Skip actual empty lines within headers

        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            setError("Malformed header line: Missing colon.");
            return;
        }

        std::string name = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);

        StringUtils::trim(name);
        StringUtils::trim(value);
        
        std::string canonicalName = name;
        for (size_t i = 0; i < canonicalName.length(); ++i) {
            canonicalName[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(canonicalName[i])));
        }
        _request.headers[canonicalName] = value;
    }

    // Process Content-Length header
    std::string contentLengthStr = _request.getHeader("content-length");
    if (!contentLengthStr.empty()) {
        try {
            _request.expectedBodyLength = StringUtils::stringToLong(contentLengthStr);
        } catch (const std::exception& e) {
            setError("Invalid Content-Length header: " + std::string(e.what()));
            return;
        }
    } else {
        if (_request.method == "POST") {
            setError("Content-Length header missing for POST request.");
            return;
        }
    }
    
    // Consume the parsed headers and the DOUBLE_CRLF from the buffer
    consumeBuffer(double_crlf_pos + DOUBLE_CRLF.length());

    // Determine the next state
    if (_request.method == "POST" && _request.expectedBodyLength > 0) {
        _request.currentState = HttpRequest::RECV_BODY;
    } else {
        _request.currentState = HttpRequest::COMPLETE;
    }

    // Check for extraneous data after request completion
    if (_request.currentState == HttpRequest::COMPLETE && !_buffer.empty()) {
        setError("Extraneous data after end of headers for request with no body.");
        return;
    }
}

// parsing the request body (optionnal), last part of a request
void HttpRequestParser::parseBody() {
    // 1 - non blocking guard : check if the entire body is in the buffer
    if (_buffer.size() < _request.expectedBodyLength) {
        return; // not enough data yet, wait for more
    }

    // 2 - copy the buffer into the HttpRequest's body vector
    _request.body.insert(_request.body.end(), _buffer.begin(), _buffer.begin() + _request.expectedBodyLength);
    
    // 3 - consume the parsed body data from the buffer
    consumeBuffer(_request.expectedBodyLength);

    // 4 - update parsing state
    _request.currentState = HttpRequest::COMPLETE;

    // CHANGE: Check for extraneous data after the body.
    if (!_buffer.empty()) { // If there's anything left in the buffer after body is consumed
        setError("Extraneous data after end of body.");
        return;
    }
}

// Decomposes the URI into path and query parameters (/search?key1=value1&key2=value2&key3=value3)
void HttpRequestParser::decomposeURI() {
    // 1 - check for presence of query string
    size_t query_pos = _request.uri.find('?');
    if (query_pos != std::string::npos) { // query string found
        // 2 - extract query path
        _request.path = _request.uri.substr(0, query_pos);

        // 3 - extract raw query string
        std::string queryString = _request.uri.substr(query_pos + 1);
        
        // 4 - prepare to parse query
        std::istringstream qss(queryString); // steam to read
        std::string pair; // temporary string for each pair

        // 5 - loop through each key=value pair (delimited by '&')
        while (std::getline(qss, pair, '&')) {
            // 6 - find delimiter separator '='
            size_t eq_pos = pair.find('=');
            if (eq_pos != std::string::npos) { // pair found
                // 7 - extract key/value
                std::string key = pair.substr(0, eq_pos);
                std::string value = pair.substr(eq_pos + 1);
                
                // 8 - store pair
                _request.queryParams[key] = value;
            } else { // key only
                // 9 - store lonely key with empty string
                _request.queryParams[pair] = "";
            }
        }
    } else { // no query string
        // 10 - entire uri is the path
        _request.path = _request.uri;
    }
}

// Main parsing function: drives the state machine
// This function ensures progress is made or returns control if more data is needed.
void HttpRequestParser::parse() {
    size_t prev_buffer_size;      // Store buffer size before parsing attempt
    HttpRequest::ParsingState prev_state; // Store current state before parsing attempt

    // The loop continues as long as the request is not complete or in an error state.
    // It also needs to make sure that progress is being made in each iteration.
    // If a parsing function returns because it's waiting for more data (no state change, no buffer consumption),
    // then this loop must break to return control to the caller.
    while (_request.currentState != HttpRequest::COMPLETE && _request.currentState != HttpRequest::ERROR) {
        prev_buffer_size = _buffer.size();      // Capture buffer size at start of iteration
        prev_state = _request.currentState;      // Capture state at start of iteration

        switch (_request.currentState) {
            case HttpRequest::RECV_REQUEST_LINE:
                parseRequestLine();
                break;
            case HttpRequest::RECV_HEADERS:
                parseHeaders();
                break;
            case HttpRequest::RECV_BODY:
                parseBody();
                break;
            case HttpRequest::COMPLETE: // These cases should ideally not be reached
            case HttpRequest::ERROR:    // if the loop condition is correct,
                return;                 // but added 'return' as a safeguard.
        }

        // Check for progress after calling a parsing function.
        // If the buffer size is the same AND the state hasn't changed,
        // it means the parsing function returned because it's waiting for more data.
        // In this scenario, we MUST break the internal while loop to prevent a hang.
        if (_buffer.size() == prev_buffer_size && _request.currentState == prev_state) {
            break; // No progress made, current state needs more data. Exit internal loop.
        }
    }
}

bool HttpRequestParser::isComplete() const {
    return _request.currentState == HttpRequest::COMPLETE;
}

bool HttpRequestParser::hasError() const {
    return _request.currentState == HttpRequest::ERROR;
}

// Getters
HttpRequest& HttpRequestParser::getRequest() {
    return _request;
}

const HttpRequest& HttpRequestParser::getRequest() const {
    return _request;
}

// Reset the parser for a new request (e.g., for Keep-Alive)
void HttpRequestParser::reset() {
    _request = HttpRequest(); // Re-initialize HttpRequest to default state
    _buffer.clear(); // Clear any remaining data in the buffer
}
