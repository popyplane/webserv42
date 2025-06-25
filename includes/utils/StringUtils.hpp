/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StringUtils.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/21 15:27:27 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 18:20:55 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STRING_UTILS_HPP
# define STRING_UTILS_HPP

#include <string>   // For std::string
#include <vector>   // For std::vector
#include <sstream>  // For std::istringstream and std::ostringstream (used in .cpp)
#include <stdexcept> // For std::invalid_argument (used in .cpp)
#include <cctype>   // For std::isdigit, std::tolower (used in .cpp)
#include <algorithm> // For std::transform

// The 'namespace' keyword is used to organize code into logical groups,
// preventing name collisions between identifiers (like functions, classes, variables)
// that might have the same name but come from different libraries or parts of your project.
// Here, all string utility functions are grouped under 'StringUtils'.
namespace StringUtils {

    // Trims leading and trailing whitespace from a string.
    // Whitespace characters include space, tab, newline, carriage return, form feed, vertical tab.
    // Example: "  hello world  \n" -> "hello world"
    void trim(std::string& s);

    void toLower(std::string& str);

    // Case-insensitive string comparison.
    bool ciCompare(const std::string& s1, const std::string& s2);

    // Splits a string into a vector of substrings based on a single character delimiter.
    // Empty tokens are included if two delimiters are adjacent (e.g., "a,,b" split by ',' yields {"a", "", "b"}).
    // Example: "apple,banana,orange" split by ',' -> {"apple", "banana", "orange"}
    // Example: "127.0.0.1:8080" split by ':' -> {"127.0.0.1", "8080"}
    std::vector<std::string> split(const std::string& s, char delimiter);

    // Checks if an entire string consists only of digit characters.
    // Returns true if all characters are digits and the string is not empty, false otherwise.
    // Example: "123" -> true, "123a" -> false, "" -> false
    bool isDigits(const std::string& str);

    // Converts a string to a long integer.
    // Provides basic error checking:
    // - Throws std::invalid_argument if the string is empty or contains non-digit characters (after an optional sign).
    // - Does not handle overflow/underflow for extremely large numbers (beyond long limits in C++98).
    // Example: "123" -> 123L, "-45" -> -45L, "abc" -> throws
    long stringToLong(const std::string& str);

    // Converts a long integer to a string.
    // Example: 123L -> "123", -45L -> "-45"
    std::string longToString(long val);

    bool startsWith(const std::string& str, const std::string& prefix);

    bool endsWith(const std::string& str, const std::string& suffix);


} // namespace StringUtils

#endif // STRING_UTILS_HPP
