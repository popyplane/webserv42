/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/23 15:26:53 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/23 22:32:54 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream> // For print()
#include <algorithm> // For std::tolower
#include <sstream> // For internal use in print() or future debugging

// --- Enum for HTTP Methods ---
// Defined directly in HttpRequest.hpp as requested.
enum HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_DELETE,
    HTTP_UNKNOWN // For methods not recognized or not supported by the server
};

// --- Helper function to convert HttpMethod enum to string for printing ---
// You can define this in a .cpp file (e.g., in HttpRequestParser.cpp or a new HttpUtils.cpp)
// For simplicity in this direct example, it's declared here.
std::string httpMethodToString(HttpMethod method);


class HttpRequest {
public:
    // --- Request Line Components ---
    std::string method;         // e.g., "GET", "POST" - The raw string method (no enum here)
    std::string uri;            // The full URI as received, e.g., "/path/to/resource?key=value"
    std::string protocolVersion;// e.g., "HTTP/1.1"

    // --- URI Decomposed Components (Derived from 'uri' AFTER parsing is complete) ---
    std::string path;           // e.g., "/path/to/resource"
    std::map<std::string, std::string> queryParams; // e.g., {"key": "value"}

    // --- Headers ---
    // Header names will be stored in a canonical form (e.g., all lowercase) for case-insensitive lookup.
    std::map<std::string, std::string> headers;

    // --- Message Body ---
    // Use std::vector<char> for the body to handle binary data safely.
    std::vector<char> body;
    size_t expectedBodyLength; // From Content-Length header, 0 if no Content-Length or not POST

    // --- Parsing State ---
    enum ParsingState {
        RECV_REQUEST_LINE, // Currently receiving/parsing the first line
        RECV_HEADERS,      // Currently receiving/parsing headers
        RECV_BODY,         // Currently receiving/parsing the request body
        COMPLETE,          // Request successfully parsed
        ERROR              // An error occurred during parsing
    };
    ParsingState currentState;

    // --- Constructor ---
    HttpRequest();

    // --- Helper Method to get header value (case-insensitive lookup) ---
    // Converts the input 'name' to lowercase before looking it up.
    std::string getHeader(const std::string& name) const;

    // --- Helper Method for debugging ---
    void print() const;
};

#endif // HTTP_REQUEST_HPP