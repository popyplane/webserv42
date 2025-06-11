/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parserTest.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/10 16:59:34 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/11 19:31:25 by baptistevie      ###   ########.fr       */
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

// --- Minimal Error Classes (if not already defined in Lexer.hpp/Parser.hpp) ---
// IMPORTANT: Ideally, these should be defined in their respective headers (Lexer.hpp, Parser.hpp)
// and properly inherit from std::exception, and provide getLine()/getColumn() if available.
// I'm putting them here for this test to compile, but you should move them.

// Basic LexerError class
class LexerError : public std::runtime_error {
private:
    int _line;
    int _column;
public:
    // Constructor to capture message, line, and column
    LexerError(const std::string& msg, int line = 0, int column = 0)
        : std::runtime_error(msg), _line(line), _column(column) {}

    // Methods to get line and column for error reporting
    int getLine() const { return _line; }
    int getColumn() const { return _column; }
};

// Basic ParseError class (assuming you already have this or similar)
// If you have a more robust ParseError, keep that.
// class ParseError : public std::runtime_error {
// private:
//     int _line;
//     int _column;
// public:
//     // Constructor to capture message, line, and column
//     ParseError(const std::string& msg, int line = 0, int column = 0)
//         : std::runtime_error(msg), _line(line), _column(column) {}

//     // Methods to get line and column for error reporting
//     int getLine() const { return _line; }
//     int getColumn() const { return _column; }
// };

// --- End of Minimal Error Classes ---


// Helper function to print AST nodes (your existing one)
// Make sure this is globally accessible or included from a common header
void printAST(const std::vector<ASTnode*>& nodes, int indent = 0) {
    std::string indentStr(indent * 2, ' ');
    
    for (std::vector<ASTnode*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        ASTnode* node = *it;
        BlockNode* block = dynamic_cast<BlockNode*>(node);
        DirectiveNode* directive = dynamic_cast<DirectiveNode*>(node);
        
        if (block) {
            std::cout << indentStr << "Block: " << block->name << " (line " << block->line << ")" << std::endl;
            
            // Print block arguments (for location blocks, etc.)
            if (!block->args.empty()) {
                std::cout << indentStr << "  Args: ";
                for (size_t i = 0; i < block->args.size(); ++i) {
                    std::cout << "\"" << block->args[i] << "\"";
                    if (i < block->args.size() - 1) std::cout << ", ";
                }
                std::cout << std::endl;
            }
            
            // Recursively print children
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

// Struct to define a test case
struct TestCase {
    std::string name;
    std::string configContent;
    bool expectedToPass; // true if syntax is valid, false if it should throw an error
    std::string expectedErrorType; // e.g., "ParseError", "LexerError", or empty if expectedToPass
};

// Function to run a single test case
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
            std::cout << "\nResult: \033[32m✓ PASSED\033[0m (Expected to pass, and it did)" << std::endl; // Green text
            testPassed = true;
        } else {
            std::cerr << "\nResult: \033[31m✗ FAILED\033[0m (Expected to fail with " << testCase.expectedErrorType << ", but parsing succeeded)" << std::endl; // Red text
            testPassed = false;
        }

    } catch (const LexerError& e) { // Catch LexerError
        actualErrorType = "LexerError";
        std::cerr << "\nLexer Error at line " << e.getLine() << ", col " << e.getColumn() 
                  << ": " << e.what() << std::endl;
        if (!testCase.expectedToPass && testCase.expectedErrorType == actualErrorType) {
            std::cout << "Result: \033[32m✓ PASSED\033[0m (Expected LexerError, and it occurred)" << std::endl; // Green text
            testPassed = true;
        } else {
            std::cerr << "Result: \033[31m✗ FAILED\033[0m (Unexpected LexerError)" << std::endl; // Red text
            testPassed = false;
        }
    } catch (const ParseError& e) { // Catch ParseError
        actualErrorType = "ParseError";
        std::cerr << "\nParse Error at line " << e.getLine() << ", col " << e.getColumn() 
                  << ": " << e.what() << std::endl;
        if (!testCase.expectedToPass && testCase.expectedErrorType == actualErrorType) {
            std::cout << "Result: \033[32m✓ PASSED\033[0m (Expected ParseError, and it occurred)" << std::endl; // Green text
            testPassed = true;
        } else {
            std::cerr << "Result: \033[31m✗ FAILED\033[0m (Unexpected ParseError)" << std::endl; // Red text
            testPassed = false;
        }
    } catch (const std::exception& e) { // Catch other standard exceptions
        actualErrorType = "std::exception";
        std::cerr << "\nUnexpected standard exception: " << e.what() << std::endl;
        std::cerr << "Result: \033[31m✗ FAILED\033[0m (Unexpected standard exception)" << std::endl; // Red text
        testPassed = false;
    } catch (...) { // Catch any other unknown errors
        actualErrorType = "UnknownError";
        std::cerr << "\nUnknown error caught." << std::endl;
        std::cerr << "Result: \033[31m✗ FAILED\033[0m (Unknown error)" << std::endl; // Red text
        testPassed = false;
    }
    
    return testPassed;
}


int main() {
    std::cout << "=== Comprehensive Config Parser Test Suite ===" << std::endl;

    // Use push_back for C++98 compatibility
    std::vector<TestCase> tests;

    // --- Valid Configurations ---
    TestCase tc1 = {"Minimal Server Block", "server {\n    listen 80;\n}\n", true, ""};
    tests.push_back(tc1);
    
    TestCase tc2 = {"Basic Server Block",
            "server {\n"
            "    listen 80;\n"
            "    server_name example.com;\n"
            "    root /var/www;\n"
            "    index index.html;\n"
            "}\n",
            true, ""};
    tests.push_back(tc2);

    TestCase tc3 = {"Server Block with Location",
            "server {\n"
            "    listen 8080;\n"
            "    location / {\n"
            "        root /var/www/html;\n"
            "        index index.html;\n"
            "    }\n"
            "}\n",
            true, ""};
    tests.push_back(tc3);

    TestCase tc4 = {"Multiple Server Blocks",
            "server {\n"
            "    listen 80;\n"
            "    server_name primary.com;\n"
            "}\n"
            "server {\n"
            "    listen 443;\n"
            "    server_name secondary.com;\n"
            "}\n",
            true, ""};
    tests.push_back(tc4);

    TestCase tc5 = {"Nested Location Blocks (Hypothetical, depends on your grammar)",
            "server {\n"
            "    location /app {\n"
            "        root /data/app;\n"
            "        location /app/files {\n"
            "            allow GET;\n"
            "        }\n"
            "    }\n"
            "}\n",
            true, ""};
    tests.push_back(tc5);

    TestCase tc6 = {"Directive with multiple arguments",
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    server_name host.com localhost;\n"
            "    error_page 404 403 /error.html;\n"
            "}\n",
            true, ""};
    tests.push_back(tc6);

    TestCase tc7 = {"Empty Block",
            "server {\n}\n",
            true, ""};
    tests.push_back(tc7);

    TestCase tc8 = {"Comments",
            "# This is a comment\n"
            "server { # Another comment\n"
            "    listen 80; # Listen on port 80\n"
            "    # This is a directive comment\n"
            "}\n",
            true, ""};
    tests.push_back(tc8);

    // --- Invalid Configurations ---
    TestCase tc9 = {"Missing Semicolon",
            "server {\n"
            "    listen 80\n" // Missing semicolon here
            "    server_name example.com;\n"
            "}\n",
            false,
            "ParseError"};
    tests.push_back(tc9);

    TestCase tc10 = {"Missing Opening Brace for Server",
            "server \n" // Missing {
            "    listen 80;\n"
            "}\n",
            false,
            "ParseError"};
    tests.push_back(tc10);

    TestCase tc11 = {"Missing Closing Brace for Server",
            "server {\n"
            "    listen 80;\n"
            "    server_name example.com;\n"
            // Missing } here
            ,
            false,
            "ParseError"};
    tests.push_back(tc11);

    TestCase tc12 = {"Missing Opening Brace for Location",
            "server {\n"
            "    listen 80;\n"
            "    location /test\n" // Missing {
            "        root /var/www;\n"
            "    }\n"
            "}\n",
            false,
            "ParseError"};
    tests.push_back(tc12);

    TestCase tc13 = {"Missing Closing Brace for Location",
            "server {\n"
            "    listen 80;\n"
            "    location /test {\n"
            "        root /var/www;\n"
            // Missing } here
            "}\n",
            false,
            "ParseError"};
    tests.push_back(tc13);

    TestCase tc14 = {"Unexpected Token (random word)",
            "server {\n"
            "    listen 80;\n"
            "    blahblah;\n" // 'blahblah' is not a valid directive/block name
            "}\n",
            false,
            "ParseError"}; // Or LexerError if 'blahblah' is not recognized as an identifier
    tests.push_back(tc14);

    TestCase tc15 = {"Directive outside of Block",
            "listen 80;\n" // Directive outside any block
            "server {\n"
            "    server_name example.com;\n"
            "}\n",
            false,
            "ParseError"};
    tests.push_back(tc15);

    TestCase tc16 = {"Block inside Directive",
            "server {\n"
            "    listen 80 {\n" // Block inside a directive
            "        invalid;\n"
            "    }\n"
            "}\n",
            false,
            "ParseError"};
    tests.push_back(tc16);

    TestCase tc17 = {"Empty File",
            "",
            true, ""}; // An empty config file is often considered valid, resulting in an empty AST.
    tests.push_back(tc17);

    TestCase tc18 = {"Only Comments",
            "# This is a comment\n"
            " # Another comment\n",
            true, ""}; // Only comments should parse fine as an empty AST
    tests.push_back(tc18);

    TestCase tc19 = {"Unclosed String in Arguments (Lexer error)",
            "server {\n"
            "    server_name \"example.com;\n" // Missing closing quote
            "}\n",
            false,
            "LexerError"}; // Assuming your lexer handles unclosed strings
    tests.push_back(tc19);

    TestCase tc20 = {"Invalid Character (Lexer error)",
            "server {\n"
            "    listen 80;\n"
            "    server_name `example.com;\n" // Backtick is usually invalid
            "}\n",
            false,
            "LexerError"}; // Assuming your lexer handles invalid characters
    tests.push_back(tc20);

    TestCase tc21 = {"Directive with no arguments (if not allowed)",
            "server {\n"
            "    listen;\n" // 'listen' typically requires arguments
            "}\n",
            false,
            "ParseError"}; // If your parser specifically checks for required args
    tests.push_back(tc21);


    int passedCount = 0;
    int totalCount = tests.size();

    for (std::vector<TestCase>::const_iterator it = tests.begin(); it != tests.end(); ++it) {
        if (runTestCase(*it)) {
            passedCount++;
        }
    }

    std::cout << "\n=== Test Suite Summary ===" << std::endl;
    std::cout << "Total Tests: " << totalCount << std::endl;
    std::cout << "Passed:      " << passedCount << std::endl;
    std::cout << "Failed:      " << (totalCount - passedCount) << std::endl;
    std::cout << "==========================" << std::endl;

    return (passedCount == totalCount) ? 0 : 1;
}


