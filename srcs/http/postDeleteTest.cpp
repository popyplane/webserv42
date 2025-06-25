/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   postDeleteTest.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/25 14:48:34 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 15:43:35 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/RequestDispatcher.hpp"
#include "../../includes/config/ConfigLoader.hpp"
#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/config/ServerStructures.hpp"
#include "../../includes/config/Lexer.hpp"
#include "../../includes/config/Parser.hpp"
#include "../../includes/utils/StringUtils.hpp" // Now includes StringUtils::longToString
#include "../../includes/http/HttpResponse.hpp"
#include "../../includes/http/HttpRequestHandler.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <fstream> // For file operations to set up/clean up test files
#include <cstdlib> // For system() call (unlink, etc.)
#include <sys/stat.h> // For stat() and mkdir()
#include <algorithm> // For std::min

// FORWARD DECLARATION of readFile (assuming it's in Lexer.cpp)
bool readFile(const std::string &fileName, std::string &out);

// Helper to convert long to string (needed for Content-Length header)
std::string longToString(long val) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

// Helper function to create a mock HttpRequest
HttpRequest createMockRequest(const std::string& method, const std::string& uri, const std::string& hostHeader,
                              const std::string& body = "", const std::string& contentType = "", long contentLength = -1) {
    HttpRequest req;
    req.method = method;
    req.uri = uri;
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        req.path = uri.substr(0, query_pos);
    } else {
        req.path = uri;
    }
    req.protocolVersion = "HTTP/1.1";
    req.headers["host"] = hostHeader;

    if (!body.empty()) {
        req.body.assign(body.begin(), body.end());
        if (contentLength == -1) { // If contentLength not explicitly provided, use body size
            req.headers["content-length"] = longToString(body.length()); // Use local longToString
        } else {
            req.headers["content-length"] = longToString(contentLength); // Use local longToString
        }
    } else if (contentLength != -1) { // If contentLength provided but body is empty (e.g., for 413 test)
         req.headers["content-length"] = longToString(contentLength); // Use local longToString
    }

    if (!contentType.empty()) {
        req.headers["content-type"] = contentType;
    }

    req.currentState = HttpRequest::COMPLETE;
    return req;
}



// C++98-compatible helper function to find server by port and (optionally) server_name
const ServerConfig* findServer(const GlobalConfig& globalConfig, int port, const std::string& serverName) {
    for (size_t i = 0; i < globalConfig.servers.size(); ++i) {
        const ServerConfig& s = globalConfig.servers[i];
        if (s.port == port) {
            if (serverName.empty()) { // Looking for a default server on this port (no server_name specified)
                if (s.serverNames.empty()) return &s; // Server with no name
            } else { // Looking for a specific server_name
                for (size_t j = 0; j < s.serverNames.size(); ++j) {
                    if (s.serverNames[j] == serverName) return &s;
                }
            }
        }
    }
    return NULL;
}

// C++98-compatible helper function to find location by path within a server
const LocationConfig* findLocation(const ServerConfig* server, const std::string& path) {
    if (!server) return NULL;
    for (size_t i = 0; i < server->locations.size(); ++i) {
        const LocationConfig& l = server->locations[i];
        if (l.path == path) {
            return &l;
        }
    }
    return NULL;
}

// Helper to run a test case (now including handler and response checks)
bool runDispatchAndHandleTest(const std::string& testName,
                              const RequestDispatcher& dispatcher,
                              HttpRequestHandler& handler, // Pass handler by reference
                              const HttpRequest& request,
                              const std::string& clientIp, int clientPort,
                              int expectedStatusCode,
                              const std::string& expectedContentType,
                              const std::string& expectedBodyContains,
                              const std::string& expectedLocationHeader = "") { // New optional param for Location header
    std::cout << "=== Running Test: " << testName << " ===\n";
    std::cout << "  Request: " << request.method << " " << request.uri << " (Host: " << request.getHeader("host") << ")\n";
    std::cout << "  Client Conn: " << clientIp << ":" << clientPort << "\n";

    MatchedConfig matchedConfig = dispatcher.dispatch(request, clientIp, clientPort);

    // Initial check for dispatcher's output (still important!)
    if (!matchedConfig.server_config) {
        std::cerr << "FAIL: Dispatcher did not find a server for this request. Cannot proceed to handler.\n";
        return false;
    }
    
    // Now, call the request handler
    HttpResponse response = handler.handleRequest(request, matchedConfig);

    bool overallPass = true;

    // 1. Verify HTTP Status Code
    if (response.getStatusCode() != expectedStatusCode) {
        std::cerr << "FAIL: Status Code Mismatch.\n";
        std::cerr << "  Expected: " << expectedStatusCode << " " << getHttpStatusMessage(expectedStatusCode) << "\n";
        std::cerr << "  Actual:   " << response.getStatusCode() << " " << response.getStatusMessage() << "\n";
        overallPass = false;
    } else {
        std::cout << "  Status Code: " << response.getStatusCode() << " " << response.getStatusMessage() << " (PASS)\n";
    }

    // 2. Verify Content-Type Header
    std::map<std::string, std::string> headers = response.getHeaders();
    std::string actualContentType = "";
    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Type");
    if (it != headers.end()) {
        actualContentType = it->second;
    }

    // Special handling for 204 No Content, which typically has no Content-Type
    if (expectedStatusCode == 204) {
        if (!actualContentType.empty()) {
             std::cerr << "FAIL: Content-Type unexpectedly present for 204 No Content.\n";
             std::cerr << "  Actual: " << actualContentType << "\n";
             overallPass = false;
        } else {
            std::cout << "  Content-Type: (empty as expected for 204) (PASS)\n";
        }
    }
    else if (actualContentType != expectedContentType) {
        std::cerr << "FAIL: Content-Type Mismatch.\n";
        std::cerr << "  Expected: " << expectedContentType << "\n";
        std::cerr << "  Actual:   " << actualContentType << "\n";
        overallPass = false;
    } else {
        std::cout << "  Content-Type: " << actualContentType << " (PASS)\n";
    }


    // 3. Verify Body Content (substring check for partial content)
    std::string actualBody(response.getBody().begin(), response.getBody().end());
    if (expectedBodyContains != "" && actualBody.find(expectedBodyContains) == std::string::npos) {
        std::cerr << "FAIL: Body content mismatch. Expected to contain:\n" << expectedBodyContains << "\n";
        std::cerr << "  Actual body (first 200 chars):\n" << actualBody.substr(0, std::min((size_t)200, actualBody.length())) << "\n";
        overallPass = false;
    } else if (expectedBodyContains != "") {
        std::cout << "  Body contains expected text (PASS)\n";
    } else if (expectedBodyContains == "" && !actualBody.empty() && expectedStatusCode == 204) {
        std::cerr << "FAIL: Body expected to be empty for 204 No Content, but was not.\n";
        std::cerr << "  Actual body (first 200 chars):\n" << actualBody.substr(0, std::min((size_t)200, actualBody.length())) << "\n";
        overallPass = false;
    }


    // 4. Verify Location Header (for 201 Created / 3xx Redirects)
    if (!expectedLocationHeader.empty()) {
        std::map<std::string, std::string>::const_iterator loc_it = headers.find("Location");
        if (loc_it == headers.end()) {
            std::cerr << "FAIL: Expected Location header not found.\n";
            overallPass = false;
        } else if (loc_it->second.find(expectedLocationHeader) == std::string::npos) {
            // Check if actual location header CONTAINS the expected part (for unique filenames)
            std::cerr << "FAIL: Location header mismatch.\n";
            std::cerr << "  Expected to contain: " << expectedLocationHeader << "\n";
            std::cerr << "  Actual: " << loc_it->second << "\n";
            overallPass = false;
        } else {
            std::cout << "  Location header contains expected text (PASS)\n";
        }
    }


    if (overallPass) {
        std::cout << "OVERALL PASS for test: " << testName << "\n";
    } else {
        std::cout << "OVERALL FAIL for test: " << testName << "\n";
    }
    std::cout << "================================\n\n";
    return overallPass;
}


int main() {
    std::ios_base::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    int passed_tests = 0;
    int total_tests = 0;

    // --- 1. Load Configuration ---
    std::vector<std::string> configFilesPaths;
    configFilesPaths.push_back("configs/minimal.conf");
    configFilesPaths.push_back("configs/basic.conf"); // basic.conf has /upload and /php locations

    GlobalConfig globalConfig;
    std::vector<ASTnode*> accumulatedAstNodes;

    try {
        for (size_t i = 0; i < configFilesPaths.size(); ++i) {
            std::string fileContent;
            std::cout << "DEBUG: Reading config file: " << configFilesPaths[i] << "\n";
            if (!readFile(configFilesPaths[i], fileContent)) {
                throw std::runtime_error("Failed to read config file: " + configFilesPaths[i]);
            }
            Lexer lexer(fileContent);
            std::vector<token> tokens = lexer.getTokens();
            std::cout << "DEBUG: Lexer produced " << tokens.size() << " tokens from " << configFilesPaths[i] << "\n";
            
            Parser parser(tokens);
            std::vector<ASTnode*> currentFileAsts = parser.parse();
            std::cout << "DEBUG: Parser produced " << currentFileAsts.size() << " AST root nodes from " << configFilesPaths[i] << "\n";
            
            accumulatedAstNodes.insert(accumulatedAstNodes.end(), currentFileAsts.begin(), currentFileAsts.end());
        }
        std::cout << "DEBUG: Total AST root nodes accumulated: " << accumulatedAstNodes.size() << "\n";

        ConfigLoader loader;
        globalConfig.servers = loader.loadConfig(accumulatedAstNodes);
        std::cout << "Configuration loaded successfully with " << globalConfig.servers.size() << " servers.\n\n";

    } catch (const std::exception& e) {
        std::cerr << "Error during config loading: " << e.what() << "\n";
        for (size_t i = 0; i < accumulatedAstNodes.size(); ++i) {
            delete accumulatedAstNodes[i];
        }
        return 1;
    }

    for (size_t i = 0; i < accumulatedAstNodes.size(); ++i) {
        delete accumulatedAstNodes[i];
    }
    accumulatedAstNodes.clear();

    // --- 2. Instantiate RequestDispatcher and HttpRequestHandler ---
    RequestDispatcher dispatcher(globalConfig);
    HttpRequestHandler handler;

    // --- 3. Identify specific ServerConfig and LocationConfig pointers from loaded config ---
    const ServerConfig* server_example_com_8080 = findServer(globalConfig, 8080, "example.com");
    if (!server_example_com_8080) {
        std::cerr << "ERROR: Failed to find example.com:8080 server. Cannot run POST/DELETE tests.\n";
        return 1;
    }
    const LocationConfig* loc_8080_upload = findLocation(server_example_com_8080, "/upload");
    if (!loc_8080_upload) {
        std::cerr << "ERROR: Failed to find /upload location. Cannot run POST tests.\n";
        return 1;
    }

    // --- 4. Define and Run Test Cases ---

    // === POST Tests ===

    // TC11: Successful file upload
    total_tests++;
    std::string uploadBody1 = "This is a test file for upload functionality.";
    HttpRequest req11 = createMockRequest("POST", "/upload", "example.com", uploadBody1, "text/plain", uploadBody1.length());
    if (runDispatchAndHandleTest("TC11: Successful file upload (text/plain)",
                                 dispatcher, handler, req11, "127.0.0.1", 8080,
                                 201, "text/html", "File uploaded successfully", "/upload/")) { // Check for /upload/ in Location header
        passed_tests++;
    }

    // TC12: File upload with custom filename from Content-Disposition
    total_tests++;
    std::string uploadBody2 = "Content for my_custom_file.txt";
    HttpRequest req12 = createMockRequest("POST", "/upload", "example.com", uploadBody2, "text/plain");
    req12.headers["content-length"] = longToString(uploadBody2.length());
    req12.headers["content-type"] = "text/plain";
    req12.headers["content-disposition"] = "form-data; name=\"file\"; filename=\"my_custom_file.txt\"";
    if (runDispatchAndHandleTest("TC12: File upload with custom filename",
                                 dispatcher, handler, req12, "127.0.0.1", 8080,
                                 201, "text/html", "File uploaded successfully", "/upload/my_custom_file.txt")) {
        passed_tests++;
    }

    // TC13: POST with Payload Too Large
    total_tests++;
    // We assume max_body_size is 10M (10485760 bytes) as per basic.conf.
    // Create a request with a Content-Length header exceeding this.
    HttpRequest req13 = createMockRequest("POST", "/upload", "example.com", "", "application/octet-stream", 10485761); // 10MB + 1 byte
    if (runDispatchAndHandleTest("TC13: POST with Payload Too Large (413)",
                                 dispatcher, handler, req13, "127.0.0.1", 8080,
                                 413, "text/html", "Payload Too Large")) {
        passed_tests++;
    }

    // TC14: POST to a non-writable upload directory (will require manual setup)
    // To test this: Before running the test, manually chmod 000 /Users/baptistevieilhescaze/dev/webserv42/www/uploads
    // Then after the test, chmod 777 back.
    total_tests++;
    std::cout << "WARNING: For TC14, you need to manually set `chmod 000 /Users/baptistevieilhescaze/dev/webserv42/www/uploads` BEFORE running this test, and `chmod 777` AFTER.\n";
    HttpRequest req14 = createMockRequest("POST", "/upload", "example.com", "This should fail.", "text/plain");
    req14.headers["content-length"] = longToString(req14.body.size());
    if (runDispatchAndHandleTest("TC14: POST to non-writable upload directory (403)",
                                 dispatcher, handler, req14, "127.0.0.1", 8080,
                                 403, "text/html", "Forbidden")) {
        passed_tests++;
    }
    // If you manually set permissions, set them back here for subsequent tests/runs:
    // system("chmod 777 /Users/baptistevieilhescaze/dev/webserv42/www/uploads");


    // === DELETE Tests ===

    // TC15: DELETE a non-existent file (expect 404 Not Found)
    total_tests++;
    std::string nonExistentFilePath = "/Users/baptistevieilhescaze/dev/webserv42/www/uploads/non_existent_file.txt";
    // Ensure it doesn't exist before test
    if (std::remove(nonExistentFilePath.c_str()) == 0) {
        std::cout << "INFO: Cleaned up " << nonExistentFilePath << " before test TC15.\n";
    }
    HttpRequest req15 = createMockRequest("DELETE", "/upload/non_existent_file.txt", "example.com"); // Changed /uploads to /upload
    if (runDispatchAndHandleTest("TC15: DELETE non-existent file (404)",
                                 dispatcher, handler, req15, "127.0.0.1", 8080,
                                 404, "text/html", "Not Found")) {
        passed_tests++;
    }

    // TC16: Successful file DELETE
    total_tests++;
    std::string testDeleteFilePath = "/Users/baptistevieilhescaze/dev/webserv42/www/uploads/file_to_delete.txt";
    // Create the file to be deleted
    std::ofstream ofs(testDeleteFilePath.c_str());
    if (!ofs.is_open()) {
        std::cerr << "ERROR: Failed to create file_to_delete.txt for TC16 setup. Skipping test.\n";
    } else {
        ofs << "This file will be deleted.";
        ofs.close();
        HttpRequest req16 = createMockRequest("DELETE", "/upload/file_to_delete.txt", "example.com"); // Changed /uploads to /upload
        if (runDispatchAndHandleTest("TC16: Successful file DELETE (204)",
                                     dispatcher, handler, req16, "127.0.0.1", 8080,
                                     204, "", "")) { // No content-type or body for 204 typically
            passed_tests++;
        }
    }


    // TC17: DELETE a file without write permissions (expect 403 Forbidden)
    total_tests++;
    std::string noPermsFilePath = "/Users/baptistevieilhescaze/dev/webserv42/www/uploads/no_perms_delete.txt";
    std::ofstream ofs2(noPermsFilePath.c_str());
    if (!ofs2.is_open()) {
        std::cerr << "ERROR: Failed to create no_perms_delete.txt for TC17 setup. Skipping test.\n";
    } else {
        ofs2 << "This file should not be deleted.";
        ofs2.close();
        // Set read-only permissions for owner, and no permissions for group/others
        chmod(noPermsFilePath.c_str(), S_IRUSR); // Equivalent to 0400
        std::cout << "WARNING: For TC17, file " << noPermsFilePath << " has been set to read-only.\n";
        
        HttpRequest req17 = createMockRequest("DELETE", "/upload/no_perms_delete.txt", "example.com"); // Changed /uploads to /upload
        if (runDispatchAndHandleTest("TC17: DELETE file without write permissions (403)",
                                     dispatcher, handler, req17, "127.0.0.1", 8080,
                                     403, "text/html", "Forbidden")) {
            passed_tests++;
        }
        // Cleanup: Try to set write permissions back and then remove
        chmod(noPermsFilePath.c_str(), S_IRWXU); // Make it writable for deletion
        std::remove(noPermsFilePath.c_str()); // Clean up file after test
    }


    std::cout << "\n=== POST & DELETE Test Summary ===\n";
    std::cout << "Total Tests: " << total_tests << "\n";
    std::cout << "Passed: " << passed_tests << "\n";
    std::cout << "Failed: " << (total_tests - passed_tests) << "\n";

    return (passed_tests == total_tests) ? 0 : 1;
}
