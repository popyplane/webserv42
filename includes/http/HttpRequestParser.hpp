/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequestParser.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/23 15:26:50 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/23 16:08:48 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_PARSER_HPP
# define HTTP_REQUEST_PARSER_HPP

#include "HttpRequest.hpp" // Now contains HttpMethod enum
#include <vector>
#include <string>

// Define CRLF for consistency
const std::string CRLF = "\r\n";
const std::string DOUBLE_CRLF = "\r\n\r\n";

class HttpRequestParser {
private:
    HttpRequest         _request;
    std::vector<char>   _buffer; // Buffer to accumulate incoming raw data

    // Private helper functions for parsing stages
    void parseRequestLine();
    void parseHeaders();
    void parseBody();
    void decomposeURI(); // Separates URI into path and query parameters

    // Helper to find a substring (pattern) within a std::vector<char>
    size_t findInVector(const std::string& pattern);
    // Helper to remove parsed data from the beginning of the buffer
    void consumeBuffer(size_t count);
    // Helper to set the error state and potentially log a message
    void setError(const std::string& msg);

public:
    HttpRequestParser();
    ~HttpRequestParser(); // If any dynamic allocations, manage them here

    // Appends new raw data to the internal buffer
    void appendData(const char* data, size_t len);

    // Attempts to parse the request based on the current state and buffered data
    void parse();

    // Check if parsing is complete
    bool isComplete() const;
    // Check if parsing encountered an error
    bool hasError() const;

    // Get the parsed HttpRequest object
    HttpRequest& getRequest();
    const HttpRequest& getRequest() const; // Const version

    // Reset the parser for a new request (e.g., for Keep-Alive)
    void reset();
};

#endif // HTTP_REQUEST_PARSER_HPP