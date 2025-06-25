/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/25 09:50:26 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 09:55:47 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_RESPONSE_HPP
# define HTTP_RESPONSE_HPP

#include <string>
#include <vector>
#include <map>
#include <sstream> // For building the response string
#include <ctime>   // For generating Date header

// Helper function to get HTTP status message for a given code
std::string getHttpStatusMessage(int statusCode);

// Helper function to get MIME type based on file extension
// (Will likely be defined in HttpRequestHandler.cpp or a new HttpUtils.cpp)
std::string getMimeType(const std::string& filePath);

/**
 * @brief Represents an HTTP response to be sent back to a client.
 * Encapsulates the status line, headers, and response body.
 */
class HttpResponse {
public:
    /**
     * @brief Constructor for HttpResponse.
     * Initializes with default protocol version and common headers.
     */
    HttpResponse();

    /**
     * @brief Destructor.
     */
    ~HttpResponse();

    /**
     * @brief Sets the HTTP status of the response.
     * Automatically sets the status message based on common HTTP status codes.
     * @param code The HTTP status code (e.g., 200, 404).
     */
    void setStatus(int code);

    /**
     * @brief Adds or updates a header in the response.
     * @param name The name of the header (e.g., "Content-Type", "Server").
     * @param value The value of the header.
     */
    void addHeader(const std::string& name, const std::string& value);

    /**
     * @brief Sets the response body from a string.
     * Automatically sets the Content-Length header.
     * @param content A string containing the response body.
     */
    void setBody(const std::string& content);

    /**
     * @brief Sets the response body from a vector of characters (for binary data).
     * Automatically sets the Content-Length header.
     * @param content A vector of characters containing the response body.
     */
    void setBody(const std::vector<char>& content);

    /**
     * @brief Generates the complete raw HTTP response string, ready to be sent over a socket.
     * This includes the status line, all headers, and the body, separated by CRLF.
     * @return A string representing the full HTTP response.
     */
    std::string toString() const;

    // --- Getters for Response Components (Optional, but good for debugging/inspection) ---
    int getStatusCode() const { return _statusCode; }
    const std::string& getStatusMessage() const { return _statusMessage; }
    const std::string& getProtocolVersion() const { return _protocolVersion; }
    const std::map<std::string, std::string>& getHeaders() const { return _headers; }
    const std::vector<char>& getBody() const { return _body; }


private:
    std::string _protocolVersion;    // e.g., "HTTP/1.1"
    int         _statusCode;         // e.g., 200, 404
    std::string _statusMessage;      // e.g., "OK", "Not Found"
    std::map<std::string, std::string> _headers; // Header names are typically canonical (e.g., "Content-Type")
    std::vector<char> _body;         // Use std::vector<char> for the body to handle binary data safely.

    // Helper to generate current GMT date/time for the "Date" header
    std::string getCurrentGmTime() const;

    // Default headers that should always be present unless explicitly overridden
    void setDefaultHeaders();
};

#endif // HTTP_RESPONSE_HPP
