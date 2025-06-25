/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgiTestMain.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/25 22:38:32 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 23:03:30 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/CGIHandler.hpp"
#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/http/HttpResponse.hpp" // For HttpResponse definition
#include "../../includes/config/ServerStructures.hpp" // For ServerConfig and LocationConfig
#include "../../includes/utils/StringUtils.hpp" // For StringUtils::longToString etc.

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sys/stat.h> // For stat, mkdir
#include <unistd.h>   // For access, chdir
#include <fstream>    // For std::ofstream
#include <cstring>    // For strerror
#include <errno.h>    // For errno
#include <sys/time.h> // For usleep

// Helper to create directories if they don't exist
void create_directory_if_not_exists(const std::string& path) {
    struct stat st; // Corrected: removed = {0} initializer
    if (stat(path.c_str(), &st) == -1) {
        // Attempt to create with rwxr-xr-x permissions
        if (mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0) {
            std::cout << "INFO: Created directory: " << path << std::endl;
        } else {
            std::cerr << "ERROR: Failed to create directory: " << path << " - " << strerror(errno) << std::endl;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        std::cerr << "ERROR: Path exists but is not a directory: " << path << std::endl;
    }
}

// Helper to create a dummy test file for CGI script
void create_cgi_script_file(const std::string& path, const std::string& content) {
    std::ofstream ofs(path.c_str());
    if (ofs.is_open()) {
        ofs << content;
        ofs.close();
        std::cout << "INFO: Created CGI script file: " << path << std::endl;
        // Make it executable
        chmod(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    } else {
        std::cerr << "ERROR: Failed to create CGI script file: " << path << std::endl;
    }
}

// A simple function to run a CGI test scenario
bool runCGITest(const std::string& testName,
                const HttpRequest& request,
                const ServerConfig& serverConfig,
                const LocationConfig& locationConfig) {

    std::cout << "\n=== Running CGI Test: " << testName << " ===\n";
    std::cout << "Request: " << request.method << " " << request.uri << std::endl;

    try {
        CGIHandler cgiHandler(request, &serverConfig, &locationConfig);

        if (cgiHandler.getState() == CGIState::CGI_PROCESS_ERROR) {
            std::cerr << "FAIL: CGIHandler constructor failed to initialize. Check CGI paths and config." << std::endl;
            return false;
        }

        if (!cgiHandler.start()) {
            std::cerr << "FAIL: CGIHandler::start() failed." << std::endl;
            return false;
        }

        // Simulate main loop's non-blocking I/O with blocking reads/writes for testing
        // In a real server, these would be driven by poll/select
        
        // Loop while the CGI operation is not finished
        // Add a safety counter to prevent infinite loops in case of unexpected hangs
        int loop_count = 0;
        const int max_loop_iterations = 1000; // Arbitrary limit for test sanity

        while (!cgiHandler.isFinished() && loop_count < max_loop_iterations) {
            // Give a small delay to allow CGI process to start/produce output
            usleep(1000); // 1ms delay

            cgiHandler.pollCGIProcess(); // Always poll the process

            // Handle writing to CGI stdin (for POST body)
            // Only attempt to write if the handler is in the WRITING_INPUT state AND the pipe is still open
            if (cgiHandler.getState() == CGIState::WRITING_INPUT && cgiHandler.getWriteFd() != -1) {
                cgiHandler.handleWrite();
            }

            // Handle reading from CGI stdout
            // Can read if in READING_OUTPUT state OR still WRITING_INPUT (CGI might send early output) AND the pipe is still open
            if ((cgiHandler.getState() == CGIState::READING_OUTPUT ||
                 cgiHandler.getState() == CGIState::WRITING_INPUT ||
                 cgiHandler.getState() == CGIState::PROCESSING_OUTPUT) && cgiHandler.getReadFd() != -1) {
                cgiHandler.handleRead();
            }
            loop_count++;
        }
        
        if (loop_count >= max_loop_iterations) {
            std::cerr << "FAIL: CGI test loop exceeded max iterations. Possible hang or infinite loop in CGIHandler." << std::endl;
            cgiHandler.setTimeout(); // Force timeout if it appears hung
        }


        std::cout << "CGI execution finished. Final state: " << cgiHandler.getState() << std::endl;

        // Verify the final state is a success state for the test to pass
        if (cgiHandler.getState() == CGIState::COMPLETE || cgiHandler.getState() == CGIState::PROCESSING_OUTPUT) {
            const HttpResponse& response = cgiHandler.getHttpResponse();
            std::cout << "--- CGI Response from Handler ---" << std::endl;
            std::cout << response.toString() << std::endl;
            std::cout << "----------------------------------" << std::endl;

            // Basic checks for the response (can be expanded)
            // Check for 200 OK, Content-Type, and a unique string from PHP output
            if (response.getStatusCode() == 200 &&
                response.getHeaders().count("Content-Type") &&
                response.getHeaders().at("Content-Type").find("text/html") != std::string::npos)
            {
                std::string body_str(response.getBody().begin(), response.getBody().end());
                if (body_str.find("Hello from PHP CGI!") != std::string::npos) {
                    // Check if the CGI's custom header is correctly parsed into the HttpResponse headers
                    if (response.getHeaders().count("X-CGI-Test-Header") &&
                        response.getHeaders().at("X-CGI-Test-Header") == "My Custom Value") {
                        std::cout << "PASS: Basic CGI response content, headers, and custom header verified." << std::endl;
                        return true;
                    } else {
                        std::cerr << "FAIL: Custom CGI header 'X-CGI-Test-Header' not found or value mismatch in final HttpResponse." << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "FAIL: Body content 'Hello from PHP CGI!' not found." << std::endl;
                    return false;
                }
            } else {
                std::cerr << "FAIL: Basic CGI response status code, or Content-Type mismatch." << std::endl;
                return false;
            }
        } else {
            std::cerr << "FAIL: CGI execution ended in error state: " << cgiHandler.getState() << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "FAIL: Exception caught during CGI test: " << e.what() << std::endl;
        return false;
    }
}


int main() {
    // Setup environment for tests
    // Using relative paths now that Makefile handles absolute root directories
    create_directory_if_not_exists("www/html/php");
    create_directory_if_not_exists("www/uploads"); // Ensure uploads dir exists for future POST tests

    // Create the test PHP script
    // This is useful for standalone test execution, even if Makefile also ensures it.
    create_cgi_script_file("www/html/php/test.php",
                           "<?php\n"
                           "header(\"Content-Type: text/html\");\n"
                           "header(\"X-CGI-Test-Header: My Custom Value\");\n"
                           "echo \"<html><body>\";\n"
                           "echo \"<h1>Hello from PHP CGI!</h1>\";\n"
                           "echo \"<p>Request Method: \" . $_SERVER['REQUEST_METHOD'] . \"</p>\";\n"
                           "echo \"<p>Script Name: \" . $_SERVER['SCRIPT_NAME'] . \"</p>\";\n"
                           "echo \"<p>Path Info: \" . $_SERVER['PATH_INFO'] . \"</p>\";\n"
                           "echo \"<p>Query String: \" . $_SERVER['QUERY_STRING'] . \"</p>\n\";\n"
                           "if ($_SERVER['REQUEST_METHOD'] == 'POST') {\n"
                           "    echo \"<h2>POST Body Received:</h2>\";\n"
                           "    $input = file_get_contents('php://stdin');\n"
                           "    if ($input === false) { echo \"<p>Error reading POST body.</p>\"; } \n"
                           "    elseif (empty($input)) { echo \"<p>No POST body provided.</p>\"; }\n"
                           "    else { echo \"<pre>\" . htmlspecialchars($input) . \"</pre>\"; }\n"
                           "}\n"
                           "echo \"<h2>All Environment Variables:</h2>\";\n"
                           "echo \"<pre>\";\n"
                           "foreach ($_SERVER as $key => $value) {\n"
                           "    echo $key . \" = \" . htmlspecialchars($value) . \"\\n\";\n"
                           "}\n"
                           "echo \"</pre>\";\n"
                           "echo \"</body></html>\";\n"
                           "?>");

    int passed_tests = 0;
    int total_tests = 0;

    // --- Mock Configuration Objects ---
    ServerConfig mockServer;
    mockServer.port = 8080;
    mockServer.serverNames.push_back("example.com");
    mockServer.root = "./www/html"; // Use relative path from project root for tests

    LocationConfig mockLocation;
    mockLocation.path = "/php/"; // Matches your config
    mockLocation.root = "./www/html"; // This should match server root for this test
    // Assuming php-cgi is at this common path. Adjust if yours is different.
    mockLocation.cgiExecutables[".php"] = "/opt/homebrew/bin/php-cgi";
    mockLocation.allowedMethods.push_back(HTTP_GET);
    mockLocation.allowedMethods.push_back(HTTP_POST);


    // --- Test Cases ---

    // Test 1: GET Request to CGI Script
    total_tests++;
    HttpRequest getRequest;
    getRequest.method = "GET";
    getRequest.uri = "/php/test.php?name=test&id=123";
    getRequest.path = "/php/test.php";
    getRequest.protocolVersion = "HTTP/1.1";
    getRequest.headers["host"] = "example.com";
    getRequest.currentState = HttpRequest::COMPLETE;

    if (runCGITest("TC1: GET request to CGI", getRequest, mockServer, mockLocation)) {
        passed_tests++;
    }

    // Test 2: POST Request to CGI Script with Body
    total_tests++;
    HttpRequest postRequest;
    postRequest.method = "POST";
    postRequest.uri = "/php/test.php";
    postRequest.path = "/php/test.php";
    postRequest.protocolVersion = "HTTP/1.1";
    postRequest.headers["host"] = "example.com";
    postRequest.headers["content-type"] = "application/x-www-form-urlencoded";
    std::string postBody = "key1=value1&key2=value2&data=This+is+some+post+data";
    postRequest.body.assign(postBody.begin(), postBody.end());
    postRequest.headers["content-length"] = StringUtils::longToString(postBody.length());
    postRequest.currentState = HttpRequest::COMPLETE;

    if (runCGITest("TC2: POST request to CGI with body", postRequest, mockServer, mockLocation)) {
        passed_tests++;
    }
    
    // Test 3: POST Request to CGI Script with large Body (ensure it gets fully sent)
    total_tests++;
    HttpRequest largePostRequest;
    largePostRequest.method = "POST";
    largePostRequest.uri = "/php/test.php";
    largePostRequest.path = "/php/test.php";
    largePostRequest.protocolVersion = "HTTP/1.1";
    largePostRequest.headers["host"] = "example.com";
    largePostRequest.headers["content-type"] = "text/plain";
    std::string largePostBody(1024 * 10, 'A'); // 10KB of 'A'
    largePostRequest.body.assign(largePostBody.begin(), largePostBody.end());
    largePostRequest.headers["content-length"] = StringUtils::longToString(largePostBody.length());
    largePostRequest.currentState = HttpRequest::COMPLETE;

    if (runCGITest("TC3: POST request with large body", largePostRequest, mockServer, mockLocation)) {
        passed_tests++;
    }

    std::cout << "\n=== CGI Test Summary ===\n";
    std::cout << "Total Tests: " << total_tests << "\n";
    std::cout << "Passed: " << passed_tests << "\n";
    std::cout << "Failed: " << (total_tests - passed_tests) << "\n";

    return (passed_tests == total_tests) ? 0 : 1;
}
