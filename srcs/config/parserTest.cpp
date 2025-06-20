/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parserTest.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/10 16:59:34 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/20 16:23:42 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/config/Lexer.hpp"
#include "../../includes/config/Parser.hpp"
#include "../../includes/config/ASTnode.hpp" // Make sure this is included for ASTnode types
#include <iostream>
#include <fstream>
#include <sstream> // For in-memory string streams
#include <vector>
#include <string>
#include <stdexcept> // Required for std::exception

// Helper function to print AST nodes (copied from parserTest.cpp for consistency)
void printAST(const std::vector<ASTnode*>& nodes, int indent = 0) {
    std::string indentStr(indent * 2, ' ');
    for (std::vector<ASTnode*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        ASTnode* node = *it;
        BlockNode* block = dynamic_cast<BlockNode*>(node);
        DirectiveNode* directive = dynamic_cast<DirectiveNode*>(node);
        
        if (block) {
            std::cout << indentStr << "Block: " << block->name << " (line " << block->line << ")" << std::endl;
            if (!block->args.empty()) {
                std::cout << indentStr << "  Args: ";
                for (size_t i = 0; i < block->args.size(); ++i) {
                    std::cout << "\"" << block->args[i] << "\"";
                    if (i < block->args.size() - 1) std::cout << ", ";
                }
                std::cout << std::endl;
            }
            if (!block->children.empty()) {
                std::cout << indentStr << "  Children:" << std::endl;
                printAST(block->children, indent + 2);
            }
        }
        else if (directive) {
            std::cout << indentStr << "Directive: " << directive->name << " (line " << directive->line << ")" << std::endl;
            if (!directive->args.empty()) {
                std::cout << indentStr << "  Args: ";
                for (size_t i = 0; i < directive->args.size(); ++i) {
                    std::cout << "\"" << directive->args[i] << "\"";
                    if (i < directive->args.size() - 1) std::cout << ", ";
                }
                std::cout << std::endl;
            }
        }
    }
}

// Struct to define a test case (copied for consistency)
struct TestCase {
    std::string name;
    std::string configContent;
    bool expectedToPass;
    std::string expectedErrorType;
};

// Function to run a single test case (copied for consistency)
bool runTestCase(const TestCase& testCase) {
    std::cout << "\n=== Running Test: " << testCase.name << " ===" << std::endl;
    std::cout << "--- Config Content ---\n" << testCase.configContent << std::endl;
    std::cout << "----------------------" << std::endl;

    bool testPassed = false;
    std::string actualErrorType = "";

    try {
        Lexer lexer(testCase.configContent);
        lexer.lexConf();
        std::cout << "\n--- Lexing Complete ---" << std::endl;
        lexer.dumpTokens(); // Dump tokens for inspection

        Parser parser(lexer.getTokens());
        std::vector<ASTnode*> ast = parser.parse();
        
        std::cout << "\n--- Parsing Complete ---" << std::endl;
        std::cout << "AST Structure:" << std::endl;
        printAST(ast);

        // Cleanup AST memory
        parser.cleanupAST(ast);

        if (testCase.expectedToPass) {
            std::cout << "\nResult: \033[32m✓ PASSED\033[0m (Expected to pass, and it did)" << std::endl;
            testPassed = true;
        } else {
            std::cerr << "\nResult: \033[31m✗ FAILED\033[0m (Expected to fail with " << testCase.expectedErrorType << ", but parsing succeeded)" << std::endl;
            testPassed = false;
        }

    } catch (const LexerError& e) {
        actualErrorType = "LexerError";
        std::cerr << "\nLexer Error at line " << e.getLine() << ", col " << e.getColumn() 
                  << ": " << e.what() << std::endl;
        if (!testCase.expectedToPass && testCase.expectedErrorType == actualErrorType) {
            std::cout << "Result: \033[32m✓ PASSED\033[0m (Expected LexerError, and it occurred)" << std::endl;
            testPassed = true;
        } else {
            std::cerr << "Result: \033[31m✗ FAILED\033[0m (Unexpected LexerError)" << std::endl;
            testPassed = false;
        }
    } catch (const ParseError& e) {
        actualErrorType = "ParseError";
        std::cerr << "\nParse Error at line " << e.getLine() << ", col " << e.getColumn() 
                  << ": " << e.what() << std::endl;
        if (!testCase.expectedToPass && testCase.expectedErrorType == actualErrorType) {
            std::cout << "Result: \033[32m✓ PASSED\033[0m (Expected ParseError, and it occurred)" << std::endl;
            testPassed = true;
        } else {
            std::cerr << "Result: \033[31m✗ FAILED\033[0m (Unexpected ParseError)" << std::endl;
            testPassed = false;
        }
    } catch (const std::exception& e) {
        actualErrorType = "std::exception";
        std::cerr << "\nUnexpected standard exception: " << e.what() << std::endl;
        std::cerr << "Result: \033[31m✗ FAILED\033[0m (Unexpected standard exception)" << std::endl;
        testPassed = false;
    } catch (...) {
        actualErrorType = "UnknownError";
        std::cerr << "\nUnknown error caught." << std::endl;
        std::cerr << "Result: \033[31m✗ FAILED\033[0m (Unknown error)" << std::endl;
        testPassed = false;
    }
    
    return testPassed;
}


int main() {
    std::cout << "=== Comprehensive Config Parser Stress Test Suite ===" << std::endl;

    std::vector<TestCase> tests;

    // --- Stress Test Cases ---

    // 1. Many Server Blocks (e.g., 50 servers)
    {
        std::ostringstream oss;
        for (int i = 0; i < 50; ++i) {
            oss << "server {\n"
                << "    listen " << (8000 + i) << ";\n"
                << "    server_name host" << i << ".com;\n"
                << "    root /var/www/host" << i << ";\n"
                << "}\n";
        }
        TestCase tc = {"Many Server Blocks (" + std::to_string(50) + ")", oss.str(), true, ""};
        tests.push_back(tc);
    }

    // 2. Deeply Nested Location Blocks (e.g., 10 levels deep)
    {
        std::ostringstream oss;
        oss << "server {\n    listen 80;\n";
        std::string current_indent = "    ";
        for (int i = 0; i < 10; ++i) {
            oss << current_indent << "location /level" << i << " {\n";
            current_indent += "    ";
            oss << current_indent << "index nested" << i << ".html;\n";
        }
        // Close all the nested blocks
        for (int i = 0; i < 10; ++i) {
            current_indent = current_indent.substr(0, current_indent.length() - 4);
            oss << current_indent << "}\n";
        }
        oss << "}\n";
        TestCase tc = {"Deeply Nested Location Blocks (10 levels)", oss.str(), true, ""};
        tests.push_back(tc);
    }

    // 3. Server Block with Many Directives and Long Arguments
    {
        std::ostringstream oss;
        oss << "server {\n"
            << "    listen 80;\n"
            << "    server_name very.long.server.name.example.com another.long.name.test.org;\n"
            << "    error_page 400 401 402 403 404 405 500 502 503 504 /custom_error_pages/errors.html;\n"
            << "    client_max_body_size 1024m;\n"
            << "    index default.html index.php main.html;\n"
            << "    error_log /var/log/webservice/error.log crit;\n"
            << "    root /data/web/applications/main_application_root;\n"
            << "    autoindex on;\n"
            << "    location /app/files/uploads {\n"
            << "        allowed_methods GET POST DELETE;\n"
            << "        upload_enabled on;\n"
            << "        upload_store /mnt/data/uploads/app_user_files;\n"
            << "        cgi_extension .php .py .pl;\n"
            << "        cgi_path /usr/bin/php-cgi;\n"
            << "        return 200;\n"
            << "    }\n"
            << "    location = /exact/match {\n"
            << "        return 302 /new/exact/location/page.html;\n"
            << "    }\n"
            << "}\n";
        TestCase tc = {"Complex Server Block with Many Directives", oss.str(), true, ""};
        tests.push_back(tc);
    }

    // 4. Configuration with Extensive Comments and Whitespace
    {
        std::ostringstream oss;
        oss << "# Main webserv configuration file\n\n";
        oss << "   # This is a server block\n";
        oss << "   server   {   # Server starts here\n";
        oss << "\n";
        oss << "       listen   8080;   # Listen on port 8080\n";
        oss << "\n";
        oss << "       server_name   localhost   www.example.com;   # Define server names\n";
        oss << "\n";
        oss << "       # Error pages setup\n";
        oss << "       error_page   404   /errors/404.html; \n";
        oss << "\n";
        oss << "       location   /images/   {   # Location for images\n";
        oss << "           root   /var/www/images;   \n";
        oss << "           index   default.jpg;   # Default image\n";
        oss << "       }\n\n";
        oss << "   }   # End of server block\n";
        oss << "\n\n# End of file\n";
        TestCase tc = {"Extensive Comments and Whitespace", oss.str(), true, ""};
        tests.push_back(tc);
    }

    // 5. Malformed Nested Block (Missing brace in deep level)
    {
        std::ostringstream oss;
        oss << "server {\n    listen 80;\n";
        std::string current_indent = "    ";
        for (int i = 0; i < 5; ++i) {
            oss << current_indent << "location /level" << i << " {\n";
            current_indent += "    ";
        }
        oss << current_indent << "index test.html;\n";
        // One brace missing here:
        // for (int i = 0; i < 4; ++i) { // Should be 5, but making it 4
        //     current_indent = current_indent.substr(0, current_indent.length() - 4);
        //     oss << current_indent << "}\n";
        // }
        // Force missing brace at level 4.
        current_indent = current_indent.substr(0, current_indent.length() - 4); // Back to level 4
        oss << current_indent << "}\n"; // Close level 4
        current_indent = current_indent.substr(0, current_indent.length() - 4); // Back to level 3
        oss << current_indent << "}\n"; // Close level 3
        current_indent = current_indent.substr(0, current_indent.length() - 4); // Back to level 2
        oss << current_indent << "}\n"; // Close level 2
        current_indent = current_indent.substr(0, current_indent.length() - 4); // Back to level 1
        oss << current_indent << "}\n"; // Close level 1 (but one more is needed implicitly from level 0)
        oss << "}\n"; // This one actually closes the server block, still one missing from level 5
        TestCase tc = {"Malformed Nested Block (Missing Brace in Deep Level)", oss.str(), false, "ParseError"};
        tests.push_back(tc);
    }

    // 6. Directive with Extremely Long String Argument
    {
        std::ostringstream oss;
        oss << "server {\n    listen 80;\n    error_log \"";
        for (int i = 0; i < 200; ++i) { // 200 characters long string
            oss << "a";
        }
        oss << ".log\" info;\n}\n";
        TestCase tc = {"Directive with Extremely Long String Argument", oss.str(), true, ""};
        tests.push_back(tc);
    }

    // 7. Large Number of Directives within a Single Server Block (e.g., 50 directives)
    {
            std::ostringstream oss;
        oss << "server {\n    listen 80;\n";
        for (int i = 0; i < 50; ++i) {
            oss << "    index file" << i << ".html;\n";
        }
        oss << "}\n";
        TestCase tc = {"Many Directives in Single Server Block (50)", oss.str(), true, ""};
        tests.push_back(tc);
    }

    // 8. Invalid Directive in Location Context (Expected to fail)
    {
        std::ostringstream oss;
        oss << "server {\n    listen 80;\n    location / {\n        server_name invalid.com;\n    }\n}\n";
        TestCase tc = {"Invalid Directive in Location Context", oss.str(), false, "ParseError"};
        tests.push_back(tc);
    }
    
    // 9. Invalid Directive (Missing Required Arg) in a Large Config
    {
        std::ostringstream oss;
        oss << "server {\n    listen 80;\n    server_name example.com;\n}\n"
            << "server {\n    listen;\n    server_name secondary.com;\n}\n"; // Missing arg for listen
        TestCase tc = {"Missing Required Arg in Large Config", oss.str(), false, "ParseError"};
        tests.push_back(tc);
    }


    int passedCount = 0;
    int totalCount = tests.size();

    for (std::vector<TestCase>::const_iterator it = tests.begin(); it != tests.end(); ++it) {
        if (runTestCase(*it)) {
            passedCount++;
        }
    }

    std::cout << "\n=== Stress Test Suite Summary ===" << std::endl;
    std::cout << "Total Tests: " << totalCount << std::endl;
    std::cout << "Passed:      " << passedCount << std::endl;
    std::cout << "Failed:      " << (totalCount - passedCount) << std::endl;
    std::cout << "=================================" << std::endl;

    return (passedCount == totalCount) ? 0 : 1;
}
