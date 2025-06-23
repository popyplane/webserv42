/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StringUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/21 15:27:51 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/23 14:09:42 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/utils/StringUtils.hpp" // Include the header with declarations
#include <algorithm> // For std::find_if (used in some trim implementations, though not explicitly here)
#include <cctype>    // For std::isspace, std::isdigit, std::tolower (essential)
#include <stdexcept> // For std::invalid_argument

// 'using namespace StringUtils;' is generally discouraged in headers,
// but fine in .cpp files or within functions to avoid prefixing every call.
// Here, we explicitly use StringUtils:: for clarity.

// Definition of the trim function, part of the StringUtils namespace.
std::string StringUtils::trim(const std::string& str) {
    // Find the first non-whitespace character
    size_t first = 0;
    while (first < str.length() && std::isspace(str[first])) {
        first++;
    }

    // If the string is all whitespace, return an empty string
    if (first == str.length()) {
        return "";
    }

    // Find the last non-whitespace character
    size_t last = str.length() - 1;
    while (last > first && std::isspace(str[last])) {
        last--;
    }

    // Extract the substring from the first non-whitespace to the last
    return str.substr(first, (last - first + 1));
}

// Definition of the split function.
std::vector<std::string> StringUtils::split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    // std::istringstream allows treating a std::string as an input stream,
    // which is useful for using std::getline with a delimiter.
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Definition of the isDigits function.
bool StringUtils::isDigits(const std::string& str) {
    if (str.empty()) {
        return false; // An empty string is not considered to consist only of digits
    }
    // Iterate through each character and check if it's a digit
    for (size_t i = 0; i < str.length(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(str[i]))) { // Use static_cast for safety with isdigit
            return false; // Found a non-digit character
        }
    }
    return true; // All characters are digits
}

// Definition of the stringToLong function.
long StringUtils::stringToLong(const std::string& str) {
    // Basic validation for empty string
    if (str.empty()) {
        throw std::invalid_argument("stringToLong: Empty string.");
    }

    long result = 0;
    int sign = 1;
    size_t i = 0;

    // Handle optional sign at the beginning
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }

    // Check if remaining part of the string consists only of digits.
    // This catches cases like "123a" or "--123".
    if (!StringUtils::isDigits(str.substr(i))) {
        throw std::invalid_argument("stringToLong: Contains non-digit characters after sign, or invalid sign placement.");
    }
    
    // Parse digits and build the long value
    for (; i < str.length(); ++i) {
        // Simple overflow check (for C++98, more robust checks are complex)
        // This is a basic safeguard against multiplying by 10 potentially overflowing
        // a long before the last digit is added.
        if (result > (2147483647L / 10) || (result == (2147483647L / 10) && (str[i] - '0') > (2147483647L % 10))) {
            if (sign == 1)
                throw std::out_of_range("stringToLong: Positive overflow.");
            else if (result < (-2147483648L / 10) || (result == (-2147483648L / 10) && (str[i] - '0') > (-2147483648L % 10 * -1)))
                throw std::out_of_range("stringToLong: Negative overflow.");
        }
        result = result * 10 + (str[i] - '0');
    }

    return result * sign; // Apply the sign at the end
}
