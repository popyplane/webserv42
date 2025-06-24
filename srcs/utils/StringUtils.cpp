/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StringUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/21 15:27:51 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/24 14:48:36 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/utils/StringUtils.hpp" // Include the header with declarations
#include <algorithm> // For std::find_if (not directly used in this trim, but common)
#include <cctype>    // For std::isspace, std::isdigit, std::tolower (essential)
#include <stdexcept> // For std::invalid_argument, std::out_of_range
#include <limits>    // For std::numeric_limits<long>::max/min (for portable overflow checks)
#include <cstdlib>   // For std::strtol if you ever decide to switch back

// ============================================================================
// CHANGE: Modified `trim` function to modify the string in-place (void return, non-const reference)
// This is crucial because your parser calls `StringUtils::trim(value);` expecting in-place modification.
// ============================================================================
void StringUtils::trim(std::string& s) { // CHANGE: Changed signature from 'const std::string& str' to 'std::string& s'
	// Find the first non-whitespace character from the beginning
	size_t first = 0;
	// CHANGE: Added static_cast<unsigned char> for portability with std::isspace
	while (first < s.length() && std::isspace(static_cast<unsigned char>(s[first]))) {
		first++;
	}

	// If the string contains only whitespace or is empty, clear it and return
	if (first == s.length()) {
		s.clear(); // CHANGE: Clear the string instead of returning a new empty one
		return;
	}

	// Find the last non-whitespace character from the end
	size_t last = s.length() - 1;
	// CHANGE: Added static_cast<unsigned char> for portability with std::isspace
	// Loop condition: `last > first` ensures we don't go past the first non-whitespace character
	while (last > first && std::isspace(static_cast<unsigned char>(s[last]))) {
		last--;
	}

	// Extract the substring containing only non-whitespace characters and assign it back to 's'
	// This effectively modifies the original string in place.
	s = s.substr(first, (last - first + 1)); // CHANGE: Assign the trimmed substring back to 's'
}

void StringUtils::toLower(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

// Definition of the split function.
// This function appears correct and robust for C++98.
std::vector<std::string> StringUtils::split(const std::string& s, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

// Definition of the isDigits function.
// This function is correct for its purpose: checking if a string consists *only* of digits.
// It does NOT handle signs or leading/trailing whitespace.
bool StringUtils::isDigits(const std::string& str) {
	if (str.empty()) {
		return false;
	}
	for (size_t i = 0; i < str.length(); ++i) {
		// CHANGE: Added static_cast<unsigned char> for portability with std::isdigit
		if (!std::isdigit(static_cast<unsigned char>(str[i]))) {
			return false;
		}
	}
	return true;
}

// ============================================================================
// CHANGE: Heavily revised `stringToLong` for robustness, whitespace handling,
// and portable overflow checks.
// ============================================================================
long StringUtils::stringToLong(const std::string& str) {
	// 1. Create a mutable copy and trim it.
	// This is CRUCIAL. It ensures we operate on a string without leading/trailing whitespace,
	// which simplifies subsequent parsing and prevents issues like " 11" failing isDigits.
	std::string s = str;
	StringUtils::trim(s); // CHANGE: Call the in-place trim here.

	// 2. Basic validation for empty string after trimming.
	if (s.empty()) {
		throw std::invalid_argument("stringToLong: Empty string after trimming.");
	}

	long result = 0;
	int sign = 1;
	size_t i = 0;

	// 3. Handle optional sign at the beginning.
	if (s[0] == '-') {
		sign = -1;
		i = 1;
	} else if (s[0] == '+') {
		i = 1;
	}

	// 4. Ensure there's at least one digit after the optional sign.
	// This catches cases like "", " ", "+", "-", " + ", " - ".
	if (i == s.length()) {
		 throw std::invalid_argument("stringToLong: Contains only sign, or is empty after sign.");
	}
	
	// 5. Parse digits and build the long value with robust overflow checks.
	// Use std::numeric_limits for portable max/min values of 'long'.
	long max_long = std::numeric_limits<long>::max();
	// long min_long = std::numeric_limits<long>::min();

	for (; i < s.length(); ++i) {
		// CHANGE: Check for non-digit characters *during* parsing loop.
		// This makes `stringToLong` self-validating without needing `isDigits` on substrings.
		if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
			throw std::invalid_argument("stringToLong: Non-digit character encountered after digits started.");
		}
		int digit = s[i] - '0';

		// Robust Overflow/Underflow Checks
		if (sign == 1) { // Handling positive numbers
			// Check if adding the next digit would cause overflow before multiplication
			if (result > max_long / 10 || (result == max_long / 10 && digit > max_long % 10)) {
				throw std::out_of_range("stringToLong: Positive overflow.");
			}
		} else { // Handling negative numbers (build result as positive, then apply sign at end)
				 // Check if the number, when negated, would cause underflow.
				 // This is equivalent to checking if `result * 10 + digit` exceeds `-(MIN_LONG)`.
				 // Note: -(MIN_LONG) is typically MAX_LONG + 1 (e.g., 9223372036854775808 for 64-bit long)
			if (result > max_long / 10 || (result == max_long / 10 && digit > (max_long % 10 + 1))) {
				throw std::out_of_range("stringToLong: Negative underflow.");
			}
			// Special edge case: If MIN_LONG is -2147483648 and MAX_LONG is 2147483647
			// Then -MIN_LONG (2147483648) is 1 greater than MAX_LONG.
			// When result == (MAX_LONG / 10), and digit is the specific last digit (e.g., 8 for 64-bit long)
			// that makes it MIN_LONG, this requires careful handling.
			// The `digit > (max_long % 10 + 1)` handles this for common 2's complement systems.
		}
		result = result * 10 + digit;
	}

	return result * sign; // Apply the sign at the end
}