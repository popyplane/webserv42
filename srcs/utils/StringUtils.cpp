/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StringUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/21 15:27:51 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 22:56:49 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/utils/StringUtils.hpp" // Include the header with declarations
#include <algorithm> // For std::transform (still needed for toLower, ciCompare)
#include <cctype>    // For std::isspace, std::isdigit, std::tolower (essential)
#include <stdexcept> // For std::invalid_argument, std::out_of_range
#include <limits>    // For std::numeric_limits<long>::max/min (for portable overflow checks)
#include <cstdlib>   // For std::strtol if you ever decide to switch back
#include <sstream>   // For std::istringstream and std::ostringstream

namespace StringUtils {

    // Restored your original, correct C++98 manual trim implementation.
    void trim(std::string& s) {
        size_t first = 0;
        while (first < s.length() && std::isspace(static_cast<unsigned char>(s[first]))) {
            first++;
        }

        if (first == s.length()) { // String is all whitespace
            s.clear();
            return;
        }

        size_t last = s.length() - 1;
        while (last > first && std::isspace(static_cast<unsigned char>(s[last]))) {
            last--;
        }

        s = s.substr(first, (last - first + 1));
    }

    void toLower(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    }

    // Case-insensitive string comparison
    bool ciCompare(const std::string& s1, const std::string& s2) {
        if (s1.length() != s2.length()) {
            return false;
        }
        for (size_t i = 0; i < s1.length(); ++i) {
            if (std::tolower(static_cast<unsigned char>(s1[i])) !=
                std::tolower(static_cast<unsigned char>(s2[i]))) {
                return false;
            }
        }
        return true;
    }

    std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    bool isDigits(const std::string& str) {
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

    // Checks if a string starts with a given prefix
    bool startsWith(const std::string& str, const std::string& prefix) {
        return str.length() >= prefix.length() && str.compare(0, prefix.length(), prefix) == 0;
    }

    // Checks if a string ends with a given suffix
    bool endsWith(const std::string& str, const std::string& suffix) {
        if (str.length() < suffix.length()) {
            return false;
        }
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    // Re-implementing stringToLong using stringstream for robustness as discussed.
    long stringToLong(const std::string& str) {
        std::string s = str;
        StringUtils::trim(s); // Trim the string

        if (s.empty()) {
            throw std::invalid_argument("stringToLong: Empty string after trimming.");
        }

        // Check for non-digit characters (except for an optional leading sign) before conversion
        size_t start_idx = 0;
        if (!s.empty() && (s[0] == '+' || s[0] == '-')) {
            start_idx = 1;
            if (s.length() == 1) { // string is just "+" or "-"
                throw std::invalid_argument("stringToLong: Contains only a sign.");
            }
        }
        for (size_t i = start_idx; i < s.length(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
                throw std::invalid_argument("stringToLong: Non-digit character encountered.");
            }
        }

        std::istringstream iss(s);
        long val;
        iss >> val;
        
        // Check for conversion failure (e.g., non-numeric data, overflow) or if extra characters remain
        if (iss.fail() || !iss.eof()) {
            std::ostringstream oss_err;
            oss_err << "stringToLong: Conversion failed for '" << s << "' (possible overflow/underflow or invalid format).";
            throw std::invalid_argument(oss_err.str());
        }
        return val;
    }


    // Converts a long integer to a string.
    std::string longToString(long val) {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    }

} // namespace StringUtils
