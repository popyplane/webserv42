/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/23 15:27:00 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/24 12:10:55 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/HttpRequest.hpp"
// CHANGE: Include <cctype> for static_cast<unsigned char> with isprint (and potentially isspace/tolower if not in StringUtils)
#include <cctype> // For std::isprint for safe character printing

HttpRequest::HttpRequest() : expectedBodyLength(0), currentState(RECV_REQUEST_LINE)
{}

std::string HttpRequest::getHeader(const std::string& name) const
{
	std::string lowerName = name;
	for (size_t i = 0; i < lowerName.length(); ++i) {
		lowerName[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowerName[i])));
	}
	std::map<std::string, std::string>::const_iterator it = headers.find(lowerName);
	if (it != headers.end()) {
		return (it->second);
	}
	return (""); // Return empty string if header not found
}

void HttpRequest::print() const
{
	std::cout << "--- HTTP Request ---\n";
	std::cout << "Method: " << method << "\n";
	std::cout << "URI: " << uri << "\n";
	std::cout << "Path: " << path << "\n";
	std::cout << "Protocol Version: " << protocolVersion << "\n";
	std::cout << "Query Parameters:\n";
	for (std::map<std::string, std::string>::const_iterator it = queryParams.begin(); it != queryParams.end(); ++it) {
		std::cout << "  " << it->first << " = " << it->second << "\n";
	}
	std::cout << "Headers:\n";
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		std::cout << "  " << it->first << ": " << it->second << "\n";
	}
	std::cout << "Body Length: " << body.size() << " bytes (Expected: " << expectedBodyLength << ")\n";
	// CHANGE: Add raw body byte dump for debugging
	std::cout << "Raw Body Bytes:\n";
	if (body.empty()) { // CHANGE: Added check for empty body
		std::cout << "  (Body is empty)\n";
	} else {
		for (size_t i = 0; i < body.size(); ++i) {
			// Print character if printable, otherwise a dot, and its ASCII value
			// CHANGE: Use std::isprint for more robust printable char check
			if (std::isprint(static_cast<unsigned char>(body[i]))) {
				std::cout << "  char[" << i << "]: '" << body[i] << "' (ASCII: " << static_cast<int>(body[i]) << ")\n";
			} else {
				std::cout << "  char[" << i << "]: '.' (Non-printable ASCII: " << static_cast<int>(body[i]) << ")\n";
			}
		}
	}
	// END CHANGE
	std::cout << "Current State: " << static_cast<int>(currentState) << "\n";
	std::cout << "--------------------\n";
}