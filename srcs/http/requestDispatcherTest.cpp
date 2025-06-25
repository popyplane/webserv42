/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   requestDispatcherTest.cpp                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/24 18:02:38 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 13:41:27 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/RequestDispatcher.hpp"
#include "../../includes/config/ConfigLoader.hpp"
#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/config/ServerStructures.hpp"
#include "../../includes/config/Lexer.hpp"
#include "../../includes/config/Parser.hpp"
#include "../../includes/utils/StringUtils.hpp"
// --- New includes for testing HttpRequestHandler ---
#include "../../includes/http/HttpResponse.hpp"
#include "../../includes/http/HttpRequestHandler.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <stdexcept>
#include <limits> // For numeric_limits for string::npos

// FORWARD DECLARATION of readFile (assuming it's in Lexer.cpp)
bool readFile(const std::string &fileName, std::string &out);

// Helper function to create a mock HttpRequest
HttpRequest createMockRequest(const std::string& method, const std::string& uri, const std::string& hostHeader) {
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
    req.headers["host"] = hostHeader; // Headers map uses lowercase keys by default
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

// C++98-compatible helper function to find location by path and (optionally) matchType within a server
const LocationConfig* findLocation(const ServerConfig* server, const std::string& path, const std::string& matchType) {
    if (!server) return NULL;
    for (size_t i = 0; i < server->locations.size(); ++i) {
        const LocationConfig& l = server->locations[i];
        if (l.path == path) {
            if (matchType.empty() || l.matchType == matchType) return &l;
        }
    }
    return NULL;
}

// --- UPDATED Helper to run a test case (now including handler and response checks) ---
bool runDispatchAndHandleTest(const std::string& testName,
                              const RequestDispatcher& dispatcher,
                              HttpRequestHandler& handler, // Pass handler by reference
                              const HttpRequest& request,
                              const std::string& clientIp, int clientPort,
                              int expectedStatusCode,
                              const std::string& expectedContentType,
                              const std::string& expectedBodyContains) {
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

    if (actualContentType != expectedContentType) {
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
    configFilesPaths.push_back("configs/basic.conf");

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
    // These helpers rely on the content of your minimal.conf and basic.conf
    const ServerConfig* server_port_80 = NULL;
    const ServerConfig* server_example_com_8080 = NULL;

    const LocationConfig* loc_80_root = NULL;
    const LocationConfig* loc_8080_root = NULL; // New: General root for 8080 server
    const LocationConfig* loc_8080_upload = NULL;
    const LocationConfig* loc_8080_php = NULL;
    const LocationConfig* loc_8080_images = NULL;
    const LocationConfig* loc_8080_list_dir = NULL;
    // REMOVED: const LocationConfig* loc_8080_exact = NULL; // No longer testing exact matches


    // Populate server pointers based on your actual config files
    server_port_80 = findServer(globalConfig, 80, "minimal.com"); // Assuming minimal.com added
    if (!server_port_80) server_port_80 = findServer(globalConfig, 80, ""); // Fallback if no specific name for default

    server_example_com_8080 = findServer(globalConfig, 8080, "example.com");

    // Populate location pointers for found servers
    if (server_port_80) {
        loc_80_root = findLocation(server_port_80, "/", "");
    }
    if (server_example_com_8080) {
        loc_8080_root = findLocation(server_example_com_8080, "/", ""); // Find general root for 8080 server
        loc_8080_upload = findLocation(server_example_com_8080, "/upload", "");
        loc_8080_php = findLocation(server_example_com_8080, "/php/", "");
        loc_8080_images = findLocation(server_example_com_8080, "/images/", "");
        loc_8080_list_dir = findLocation(server_example_com_8080, "/list_dir/", "");
        // REMOVED: loc_8080_exact = findLocation(server_example_com_8080, "/exact", "=");
    }

    // --- Validate that critical pointers were found ---
    // Adjusted validation to reflect removed exact match and added general 8080 root
    if (!server_port_80 || !loc_80_root ||
        !server_example_com_8080 || !loc_8080_root || !loc_8080_upload || !loc_8080_php ||
        !loc_8080_images || !loc_8080_list_dir) {
        std::cerr << "ERROR: Failed to find all expected server/location pointers from loaded config.\n";
        std::cerr << "       Verify config files (minimal.conf, basic.conf) match test expectations.\n";
        std::cerr << "       Loaded servers count: " << globalConfig.servers.size() << "\n";
        std::cerr << "       Missing (current state of pointers):\n";
        if (!server_port_80) std::cerr << " - server_port_80\n";
        if (!loc_80_root) std::cerr << " - loc_80_root\n";
        if (!server_example_com_8080) std::cerr << " - server_example_com_8080\n";
        if (!loc_8080_root) std::cerr << " - loc_8080_root\n"; // New check
        if (!loc_8080_upload) std::cerr << " - loc_8080_upload\n";
        if (!loc_8080_php) std::cerr << " - loc_8080_php\n";
        if (!loc_8080_images) std::cerr << " - loc_8080_images\n";
        if (!loc_8080_list_dir) std::cerr << " - loc_8080_list_dir\n";
        std::cerr << "\n";
        return 1;
    }


    // --- 4. Define and Run Test Cases ---

    // TC1: Serve index.html from server on port 80
    total_tests++;
    HttpRequest req1 = createMockRequest("GET", "/index.html", "minimal.com");
    if (runDispatchAndHandleTest("TC1: Serve index.html (port 80)",
                                 dispatcher, handler, req1, "127.0.0.1", 80,
                                 200, "text/html", "<h1>Welcome!</h1>")) {
        passed_tests++;
    }

    // TC2: Serve about.html from server on 8080 (basic.conf's root)
    total_tests++;
    HttpRequest req2 = createMockRequest("GET", "/about.html", "example.com");
    if (runDispatchAndHandleTest("TC2: Serve about.html (port 8080, basic.conf root)",
                                 dispatcher, handler, req2, "127.0.0.1", 8080,
                                 200, "text/html", "<h1>About Us</h1>")) {
        passed_tests++;
    }

    // TC3: 404 Not Found (non-existent file)
    total_tests++;
    HttpRequest req3 = createMockRequest("GET", "/nonexistent.html", "example.com");
    if (runDispatchAndHandleTest("TC3: 404 Not Found (non-existent file)",
                                 dispatcher, handler, req3, "127.0.0.1", 8080,
                                 404, "text/html", "404 Not Found Custom")) { // Expect custom 404
        passed_tests++;
    }

    // TC4: 403 Forbidden (file without read permissions)
    total_tests++;
    HttpRequest req4 = createMockRequest("GET", "/protected_file.txt", "minimal.com");
    if (runDispatchAndHandleTest("TC4: 403 Forbidden (protected file)",
                                 dispatcher, handler, req4, "127.0.0.1", 80,
                                 403, "text/html", "Forbidden")) { // Expect generic or custom 403
        passed_tests++;
    }

    // TC5: Autoindex for directory listing
    total_tests++;
    HttpRequest req5 = createMockRequest("GET", "/list_dir/", "example.com");
    if (runDispatchAndHandleTest("TC5: Autoindex directory listing",
                                 dispatcher, handler, req5, "127.0.0.1", 8080,
                                 200, "text/html", "<h1>Index of /list_dir/</h1>")) {
        passed_tests++;
    }

    // TC6: Serve image (Content-Type check)
    total_tests++;
    HttpRequest req6 = createMockRequest("GET", "/images/logo.jpg", "example.com");
    if (runDispatchAndHandleTest("TC6: Serve image (logo.jpg)",
                                 dispatcher, handler, req6, "127.0.0.1", 8080,
                                 200, "image/jpeg", "")) { // Body content check might be hard for binary
        passed_tests++;
    }
    
    // TC7: POST request (expect 501 Not Implemented for now)
    total_tests++;
    HttpRequest req7 = createMockRequest("POST", "/upload", "example.com");
    if (runDispatchAndHandleTest("TC7: POST request (expect 501 Not Implemented)",
                                 dispatcher, handler, req7, "127.0.0.1", 8080,
                                 501, "text/html", "Not Implemented")) {
        passed_tests++;
    }

    // TC8: DELETE request (expect 501 Not Implemented for now)
    total_tests++;
    HttpRequest req8 = createMockRequest("DELETE", "/some_file.txt", "example.com");
    if (runDispatchAndHandleTest("TC8: DELETE request (expect 501 Not Implemented)",
                                 dispatcher, handler, req8, "127.0.0.1", 8080,
                                 501, "text/html", "Not Implemented")) {
        passed_tests++;
    }

    // TC9: Method Not Allowed (e.g., DELETE to a GET/POST-only location)
    total_tests++;
    HttpRequest req9 = createMockRequest("DELETE", "/php/script.php", "example.com");
    if (runDispatchAndHandleTest("TC9: Method Not Allowed (DELETE on /php)",
                                 dispatcher, handler, req9, "127.0.0.1", 8080,
                                 405, "text/html", "Method Not Allowed")) {
        passed_tests++;
    }

    // TC10: Request for a path that should be handled by the general root location for 8080 server
    total_tests++;
    HttpRequest req10 = createMockRequest("GET", "/random_page.html", "example.com");
    if (runDispatchAndHandleTest("TC10: General root location for 8080 server",
                                 dispatcher, handler, req10, "127.0.0.1", 8080,
                                 404, "text/html", "404 Not Found Custom")) { // Should serve 404 from server-level root
        passed_tests++;
    }


    std::cout << "\n=== RequestDispatcher & HttpRequestHandler Test Summary ===\n";
    std::cout << "Total Tests: " << total_tests << "\n";
    std::cout << "Passed: " << passed_tests << "\n";
    std::cout << "Failed: " << (total_tests - passed_tests) << "\n";

    return (passed_tests == total_tests) ? 0 : 1;
}