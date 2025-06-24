/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpParserTest.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/24 10:52:02 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/24 13:15:47 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/HttpRequestParser.hpp"
#include <vector>
#include <iostream>
#include <string>
#include <algorithm> // For std::min
#include <ios>       // For std::unitbuf, std::ios_base::sync_with_stdio
#include <cstring>   // For strlen for char* literals

// Helper function to run a test case
// Parameters:
//   testName: Name of the test for output.
//   chunks: A vector of strings representing data chunks to feed to the parser.
//   expectError: True if an error state is expected.
//   expectedMethod: The HTTP method expected to be parsed.
//   expectedPath: The URI path expected to be parsed.
//   expectComplete: True if the parser is expected to reach the COMPLETE state.
//                   False if a partial parse (waiting for more data) is expected.
//   expectedBody: The expected content of the request body.
bool runParserTest(const std::string& testName, const std::vector<std::string>& chunks,
                   bool expectError = false, const std::string& expectedMethod = "",
                   const std::string& expectedPath = "", bool expectComplete = true,
                   const std::string& expectedBody = "") {
    std::cout << "=== Running Test: " << testName << " ===\n" << std::flush;
    HttpRequestParser parser;

    for (size_t i = 0; i < chunks.size(); ++i) {
        std::cout << "Feeding chunk " << i + 1 << ": '" << chunks[i].substr(0, std::min((size_t)50, chunks[i].length())) << "...'\n" << std::flush;
        parser.appendData(chunks[i].c_str(), chunks[i].length());
        parser.parse();

        // If parser errors or completes prematurely, break the loop.
        if (parser.hasError() || parser.isComplete()) {
            if (parser.hasError()) {
                std::cout << "Parser entered ERROR state after chunk " << i + 1 << ".\n" << std::flush;
            } else { // parser.isComplete()
                std::cout << "Parser entered COMPLETE state after chunk " << i + 1 << ".\n" << std::flush;
            }
            break;
        }
    }

    std::cout << "--- Final Parser State ---\n" << std::flush;
    parser.getRequest().print();

    bool test_passed = true;
    if (expectError) {
        if (!parser.hasError()) {
            std::cerr << "FAIL: Expected error, but parser completed successfully or partially.\n" << std::flush;
            test_passed = false;
        } else {
            std::cout << "PASS: Parser correctly reported error.\n" << std::flush;
        }
    } else { // Expect no error
        if (parser.hasError()) {
            std::cerr << "FAIL: Unexpected error occurred.\n" << std::flush;
            test_passed = false;
        } else if (expectComplete && !parser.isComplete()) {
            std::cerr << "FAIL: Parser did not complete as expected.\n" << std::flush;
            test_passed = false;
        } else if (!expectComplete && parser.isComplete()) {
             std::cerr << "FAIL: Parser completed unexpectedly for a partial request.\n" << std::flush;
             test_passed = false;
        }
        // If expectComplete is false AND parser is not complete, it's considered a pass for a partial test
        else {
            // Further validation for expected values (only run if no immediate failure)
            if (!expectedMethod.empty() && parser.getRequest().method != expectedMethod) {
                std::cerr << "FAIL: Method mismatch. Expected '" << expectedMethod << "', got '" << parser.getRequest().method << "'.\n" << std::flush;
                test_passed = false;
            }
            if (!expectedPath.empty() && parser.getRequest().path != expectedPath) {
                std::cerr << "FAIL: Path mismatch. Expected '" << expectedPath << "', got '" << parser.getRequest().path << "'.\n" << std::flush;
                test_passed = false;
            }

            // Body content validation (only if complete and a body is expected)
            if (expectComplete && !expectedBody.empty()) {
                std::string actualBody;
                const std::vector<char>& raw_body_vec = parser.getRequest().body;
                if (!raw_body_vec.empty()) {
                    actualBody.assign(&raw_body_vec[0], raw_body_vec.size());
                }

                if (actualBody != expectedBody) {
                    std::cerr << "FAIL: Body content mismatch. Expected '" << expectedBody << "' (length: " << expectedBody.length() << "), got '" << actualBody << "' (length: " << actualBody.length() << ").\n" << std::flush;
                    test_passed = false;
                } else {
                    std::cout << "Body content matches expectations.\n" << std::flush;
                }
            }

            if (test_passed) {
                std::cout << "PASS: Parser state correct and values match expectations.\n" << std::flush;
            }
        }
    }
    std::cout << "================================\n\n" << std::flush;
    return test_passed;
}


int main() {
    // Force immediate flushing of cout and cerr for better debugging visibility
    std::ios_base::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    int passed_tests = 0;
    int total_tests = 0;

    // --- String Literal Debug Check (for internal testing of string behavior) ---
    std::string test_source_string = "{\"key\"},";
    std::string test_expected_string = "{\"key\"}";
    std::string substring_from_source = test_source_string.substr(0, 7);

    std::cout << "--- String Literal Debug Check ---\n";
    std::cout << "Source String: '" << test_source_string << "' Length: " << test_source_string.length() << "\n";
    for (size_t i = 0; i < test_source_string.length(); ++i) {
        std::cout << "  char[" << i << "]: '" << test_source_string[i] << "' (ASCII: " << static_cast<int>(test_source_string[i]) << ")\n";
    }
    std::cout << "Expected String: '" << test_expected_string << "' Length: " << test_expected_string.length() << "\n";
    for (size_t i = 0; i < test_expected_string.length(); ++i) {
        std::cout << "  char[" << i << "]: '" << test_expected_string[i] << "' (ASCII: " << static_cast<int>(test_expected_string[i]) << ")\n";
    }
    std::cout << "Substring (first 7) from source: '" << substring_from_source << "' Length: " << substring_from_source.length() << "\n";
    for (size_t i = 0; i < substring_from_source.length(); ++i) {
        std::cout << "  char[" << i << "]: '" << substring_from_source[i] << "' (ASCII: " << static_cast<int>(substring_from_source[i]) << ")\n";
    }
    if (substring_from_source == test_expected_string) {
        std::cout << "RESULT: Substring matches expected string.\n";
    } else {
        std::cout << "RESULT: Substring DOES NOT match expected string.\n";
    }
    std::cout << "----------------------------------\n\n";

    std::cout << "=== HttpRequestParser Test Suite ===\n\n";

    // --- Core Test Cases (already passing) ---

    // Test Case 1: Simple GET request, all at once
    total_tests++;
    std::vector<std::string> chunks1;
    chunks1.push_back("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
    if (runParserTest("Simple GET (One Chunk)", chunks1, false, "GET", "/")) {
        passed_tests++;
    }

    // Test Case 2: POST request with body, all at once
    total_tests++;
    std::vector<std::string> chunks2;
    chunks2.push_back("POST /submit HTTP/1.1\r\nHost: example.com\r\nContent-Length: 11\r\n\r\nHello World");
    if (runParserTest("POST with Body (One Chunk)", chunks2, false, "POST", "/submit", true, "Hello World")) {
        passed_tests++;
    }

    // Test Case 3: GET with Query String
    total_tests++;
    std::vector<std::string> chunks3;
    chunks3.push_back("GET /search?q=test&id=123 HTTP/1.1\r\nHost: example.com\r\n\r\n");
    if (runParserTest("GET with Query String", chunks3, false, "GET", "/search")) {
        passed_tests++;
        // Additional check for query parameters (uses a separate parser instance for sub-validation)
        HttpRequestParser parser_q;
        parser_q.appendData("GET /search?q=test&id=123 HTTP/1.1\r\nHost: example.com\r\n\r\n", 60);
        parser_q.parse();
        bool query_params_correct = true;
        if (parser_q.isComplete()) {
            if (!(parser_q.getRequest().queryParams.count("q") && parser_q.getRequest().queryParams["q"] == "test" &&
                  parser_q.getRequest().queryParams.count("id") && parser_q.getRequest().queryParams["id"] == "123")) {
                std::cerr << "FAIL: Query params for 'GET with Query String' are incorrect.\n";
                query_params_correct = false;
            }
        } else {
            std::cerr << "FAIL: Parser did not complete for query params sub-check.\n";
            query_params_correct = false;
        }
        if (query_params_correct) {
            std::cout << "Query params for 'GET with Query String' are correct.\n";
        }
    }


    // Test Case 4: Multi-Chunk Request (Line, Headers, Body) - Refactored to use runParserTest
    total_tests++;
    std::vector<std::string> chunks4;
    chunks4.push_back("POST /data HTTP/1.1\r\n");
    chunks4.push_back("Host: test.com\r\nContent-Type: application/json\r\nContent-Length: 7\r\n\r\n");
    chunks4.push_back("{\"key\"}"); // The body content

    if (runParserTest("Multi-Chunk Request (Line, Headers, Body)", chunks4, false, "POST", "/data", true, "{\"key\"}")) {
        passed_tests++;
    }


    // --- Malformed/Error Test Cases (already passing) ---

    // Test Case 5: Malformed Request Line - Unsupported HTTP Version
    total_tests++;
    std::vector<std::string> chunks5;
    chunks5.push_back("GET / HTTP/1.0\r\nHost: example.com\r\n\r\n");
    if (runParserTest("Malformed: Unsupported HTTP Version", chunks5, true)) {
        passed_tests++;
    }

    // Test Case 6: Malformed Headers - Missing Colon
    total_tests++;
    std::vector<std::string> chunks6;
    chunks6.push_back("GET / HTTP/1.1\r\nHost example.com\r\n\r\n");
    if (runParserTest("Malformed: Missing Header Colon", chunks6, true)) {
        passed_tests++;
    }

    // Test Case 7: Malformed Headers - Invalid Content-Length
    total_tests++;
    std::vector<std::string> chunks7;
    chunks7.push_back("POST /data HTTP/1.1\r\nHost: example.com\r\nContent-Length: abc\r\n\r\nBody");
    if (runParserTest("Malformed: Invalid Content-Length", chunks7, true)) {
        passed_tests++;
    }

    // Test Case 8: Malformed: POST without Content-Length
    total_tests++;
    std::vector<std::string> chunks8;
    chunks8.push_back("POST /data HTTP/1.1\r\nHost: example.com\r\n\r\nBody");
    if (runParserTest("Malformed: POST without Content-Length", chunks8, true)) {
        passed_tests++;
    }

    // Test Case 9: Request split mid-header
    total_tests++;
    std::vector<std::string> chunks9;
    chunks9.push_back("GET /test HTTP/1.1\r\nHost: example.co");
    chunks9.push_back("m\r\nUser-Agent: Mozill");
    chunks9.push_back("a/5.0\r\n\r\n");
    if (runParserTest("Multi-Chunk Request (Mid-Header)", chunks9, false, "GET", "/test")) {
        passed_tests++;
    }

    // Test Case 10: Partial: Just Request Line
    total_tests++;
    std::vector<std::string> chunks10;
    chunks10.push_back("GET /partial HTTP/1.1\r\n");
    if (runParserTest("Partial: Just Request Line", chunks10, false, "GET", "/partial", false)) { // expectComplete = false
        passed_tests++;
    }


    // --- NEW LIMIT TEST CASES START HERE ---

    // Test Case 11: Empty Request (just double CRLF)
    total_tests++;
    std::vector<std::string> chunks11;
    chunks11.push_back("\r\n\r\n");
    if (runParserTest("Limit: Empty Request (Double CRLF Only)", chunks11, true)) { // Should error on empty request line
        passed_tests++;
    }

    // Test Case 12: Very Long URI (2048 'a' characters)
    total_tests++;
    std::string long_uri_path(2048, 'a');
    std::vector<std::string> chunks12;
    chunks12.push_back("GET /" + long_uri_path + " HTTP/1.1\r\nHost: long.uri.test.com\r\n\r\n");
    if (runParserTest("Limit: Very Long URI", chunks12, false, "GET", "/" + long_uri_path)) {
        passed_tests++;
    }

    // Test Case 13: Very Long Header Value (2048 'x' characters)
    total_tests++;
    std::string long_header_value(2048, 'x');
    std::vector<std::string> chunks13;
    chunks13.push_back("GET / HTTP/1.1\r\nUser-Agent: " + long_header_value + "\r\nHost: long.header.test.com\r\n\r\n");
    if (runParserTest("Limit: Very Long Header Value", chunks13, false, "GET", "/")) {
        // Additional check for the header value specifically
        HttpRequestParser parser13;
        parser13.appendData(chunks13[0].c_str(), chunks13[0].length());
        parser13.parse();
        if (parser13.isComplete() && parser13.getRequest().getHeader("user-agent") == long_header_value) {
            std::cout << "User-Agent header value is correct.\n";
            passed_tests++; // Pass if main test passes and header is correct
        } else {
            std::cerr << "FAIL: User-Agent header value mismatch or parser not complete.\n";
            // Do not increment passed_tests if this sub-check fails,
            // as the runParserTest already counts the main pass/fail.
        }
    }


    // Test Case 14: Header with No Value (e.g., Custom-Header:)
    total_tests++;
    std::vector<std::string> chunks14;
    chunks14.push_back("GET / HTTP/1.1\r\nCustom-Header:\r\nHost: example.com\r\n\r\n");
    if (runParserTest("Limit: Header with No Value", chunks14, false, "GET", "/")) {
        // Additional check for header presence and empty value
        HttpRequestParser parser14;
        parser14.appendData(chunks14[0].c_str(), chunks14[0].length());
        parser14.parse();
        if (parser14.isComplete() && parser14.getRequest().headers.count("custom-header") && parser14.getRequest().getHeader("custom-header") == "") {
            std::cout << "Custom-Header with empty value is correct.\n";
            passed_tests++;
        } else {
            std::cerr << "FAIL: Custom-Header not found or value not empty.\n";
        }
    }

    // Test Case 15: Header with Multiple Colons (should be part of value)
    total_tests++;
    std::vector<std::string> chunks15;
    chunks15.push_back("GET / HTTP/1.1\r\nX-Test: value:with:colons\r\nHost: example.com\r\n\r\n");
    if (runParserTest("Limit: Header with Multiple Colons", chunks15, false, "GET", "/")) {
        // Additional check
        HttpRequestParser parser15;
        parser15.appendData(chunks15[0].c_str(), chunks15[0].length());
        parser15.parse();
        if (parser15.isComplete() && parser15.getRequest().getHeader("x-test") == "value:with:colons") {
            std::cout << "X-Test header with multiple colons is correct.\n";
            passed_tests++;
        } else {
            std::cerr << "FAIL: X-Test header value mismatch.\n";
        }
    }

    // Test Case 16: POST with Content-Length: 0 (valid case)
    total_tests++;
    std::vector<std::string> chunks16;
    chunks16.push_back("POST /upload HTTP/1.1\r\nContent-Length: 0\r\nHost: example.com\r\n\r\n");
    if (runParserTest("Limit: POST with Content-Length: 0", chunks16, false, "POST", "/upload", true, "")) { // Expected empty body
        passed_tests++;
    }

    // Test Case 17: Request with Only Line and Double CRLF (No Headers, valid)
    total_tests++;
    std::vector<std::string> chunks17;
    chunks17.push_back("GET / HTTP/1.1\r\n\r\n");
    // CHANGE: Expect this to be a PARTIAL test for parser (it will remain in RECV_HEADERS state)
    // because after parsing request line, only one \r\n remains, not \r\n\r\n
    if (runParserTest("Limit: No Headers (Line + Double CRLF)", chunks17, false, "GET", "/")) { // <-- This should still be expected to COMPLETE
        passed_tests++;
    }

    // Test Case 18: Malformed: Missing CR in Req Line CRLF (Should error as request line not found)
    total_tests++;
    std::vector<std::string> chunks18;
    chunks18.push_back("GET / HTTP/1.1\nHost: example.com\r\n\r\n"); // Missing CR after HTTP/1.1
    if (runParserTest("Malformed: Missing CR in Req Line CRLF", chunks18, true)) {
        passed_tests++;
    }

    // Test Case 19: Malformed: Missing LF in Req Line CRLF (Should error as request line not found)
    total_tests++;
    std::vector<std::string> chunks19;
    chunks19.push_back("GET / HTTP/1.1\rHost: example.com\r\n\r\n"); // Missing LF after HTTP/1.1
    if (runParserTest("Malformed: Missing LF in Req Line CRLF", chunks19, true)) {
        passed_tests++;
    }

    // Test Case 20: Malformed: Empty Header Line in Middle (Extraneous data after headers)
    total_tests++;
    std::vector<std::string> chunks20;
    chunks20.push_back("GET / HTTP/1.1\r\nHost: example.com\r\n\r\nAnother-Header: value\r\n\r\n");
    if (runParserTest("Malformed: Empty Header Line in Middle", chunks20, true)) { // CHANGE: Expected to error now
        passed_tests++;
    }


    std::cout << "\n=== Test Suite Summary ===\n";
    std::cout << "Total Tests: " << total_tests << "\n";
    std::cout << "Passed: " << passed_tests << "\n";
    std::cout << "Failed: " << (total_tests - passed_tests) << "\n";

    return (passed_tests == total_tests) ? 0 : 1;
}
