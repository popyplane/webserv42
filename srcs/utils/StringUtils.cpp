/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StringUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/21 15:27:51 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 14:01:48 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/utils/StringUtils.hpp" // Include the header with declarations
#include <algorithm> // For std::find_if (not directly used in this trim, but common)
#include <cctype>    // For std::isspace, std::isdigit, std::tolower (essential)
#include <stdexcept> // For std::invalid_argument, std::out_of_range
#include <limits>    // For std::numeric_limits<long>::max/min (for portable overflow checks)
#include <cstdlib>   // For std::strtol if you ever decide to switch back
#include <sstream> // For std::istringstream (for split function)


void StringUtils::trim(std::string& s) {
    size_t first = 0;
    while (first < s.length() && std::isspace(static_cast<unsigned char>(s[first]))) {
        first++;
    }

    if (first == s.length()) {
        s.clear();
        return;
    }

    size_t last = s.length() - 1;
    while (last > first && std::isspace(static_cast<unsigned char>(s[last]))) {
        last--;
    }

    s = s.substr(first, (last - first + 1));
}

void StringUtils::toLower(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

std::vector<std::string> StringUtils::split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool StringUtils::isDigits(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    for (size_t i = 0; i < str.length(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(str[i]))) {
            return false;
        }
    }
    return true;
}

// NEW: Checks if a string starts with a given prefix
bool StringUtils::startsWith(const std::string& str, const std::string& prefix) {
    return str.length() >= prefix.length() && str.substr(0, prefix.length()) == prefix;
}

// NEW: Checks if a string ends with a given suffix
bool StringUtils::endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}


long StringUtils::stringToLong(const std::string& str) {
    std::string s = str;
    StringUtils::trim(s);

    if (s.empty()) {
        throw std::invalid_argument("stringToLong: Empty string after trimming.");
    }

    long result = 0;
    int sign = 1;
    size_t i = 0;

    if (s[0] == '-') {
        sign = -1;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }

    if (i == s.length()) {
         throw std::invalid_argument("stringToLong: Contains only sign, or is empty after sign.");
    }
    
    long max_long = std::numeric_limits<long>::max();

    for (; i < s.length(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            throw std::invalid_argument("stringToLong: Non-digit character encountered after digits started.");
        }
        int digit = s[i] - '0';

        if (sign == 1) {
            if (result > max_long / 10 || (result == max_long / 10 && digit > max_long % 10)) {
                throw std::out_of_range("stringToLong: Positive overflow.");
            }
        } else {
            if (result > max_long / 10 || (result == max_long / 10 && digit > (max_long % 10 + 1))) {
                throw std::out_of_range("stringToLong: Negative underflow.");
            }
        }
        result = result * 10 + digit;
    }

    return result * sign;
}
