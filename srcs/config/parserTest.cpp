/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parserTest.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/10 16:59:34 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/10 17:08:00 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../includes/config/Lexer.hpp"
#include "../../includes/config/Parser.hpp"
#include <iostream>
#include <fstream>

// Helper function to print AST nodes
void printAST(const std::vector<ASTnode*>& nodes, int indent = 0) {
    std::string indentStr(indent * 2, ' ');
    
    for (std::vector<ASTnode*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        ASTnode* node = *it;
        BlockNode* block = dynamic_cast<BlockNode*>(node);
        DirectiveNode* directive = dynamic_cast<DirectiveNode*>(node);
        
        if (block) {
            std::cout << indentStr << "Block: " << block->name << " (line " << block->line << ")" << std::endl;
            
            // Print block arguments (for location blocks)
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

// Test function for a specific config file
bool testConfigFile(const std::string& filename) {
    std::cout << "\n=== Testing config file: " << filename << " ===" << std::endl;
    
    // Read file
    std::string content;
    if (!readFile(filename, content)) {
        std::cerr << "Error: Cannot read file " << filename << std::endl;
        return false;
    }
    
    std::cout << "File content:\n" << content << std::endl;
    std::cout << "--- Lexing ---" << std::endl;
    
    // Tokenize
    Lexer lexer(content);
    try {
        lexer.lexConf();
        std::cout << "Tokens:" << std::endl;
        lexer.dumpTokens();
    } catch (const std::exception& e) {
        std::cerr << "Lexer error: " << e.what() << std::endl;
        return false;
    }
    
    std::cout << "\n--- Parsing ---" << std::endl;
    
    // Parse
    Parser parser(lexer.getTokens());
    try {
        std::vector<ASTnode*> ast = parser.parse();
        std::cout << "Parse successful!" << std::endl;
        std::cout << "AST Structure:" << std::endl;
        printAST(ast);
        
        // Cleanup
        parser.cleanupAST(ast);
        return true;
        
    } catch (const ParseError& e) {
        std::cerr << "Parse Error at line " << e.getLine() << ", col " << e.getColumn() 
                  << ": " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return false;
    }
}

// Create test config files
void createTestConfigs() {
    // Simple test config
    std::ofstream simple("test_simple.conf");
    simple << "server {\n"
           << "    listen 80;\n"
           << "    server_name example.com;\n"
           << "}\n";
    simple.close();
    
    // Complex test config
    std::ofstream complex("test_complex.conf");
    complex << "server {\n"
            << "    listen 127.0.0.1:8080;\n"
            << "    server_name example.com www.test.com;\n"
            << "    client_max_body_size 10M;\n"
            << "    error_page 404 /404.html;\n"
            << "    index index.html index.php;\n"
            << "\n"
            << "    location /upload {\n"
            << "        allowed_methods GET POST;\n"
            << "        root /var/www/upload;\n"
            << "        upload_enabled on;\n"
            << "        upload_store /var/www/upload/store;\n"
            << "    }\n"
            << "\n"
            << "    location ~ \\.php$ {\n"
            << "        cgi_extension .php;\n"
            << "        cgi_path /usr/bin/php-cgi;\n"
            << "        allowed_methods GET POST;\n"
            << "    }\n"
            << "}\n";
    complex.close();
    
    // Error test config (syntax errors)
    std::ofstream error("test_error.conf");
    error << "server {\n"
          << "    listen 80\n"  // Missing semicolon
          << "    server_name example.com;\n"
          << "    location /test {\n"
          << "        root /var/www;\n"
          << "    # Missing closing brace\n";
    error.close();
}

int main() {
    std::cout << "=== Config Parser Test Suite ===" << std::endl;
    
    // Create test files
    createTestConfigs();
    
    // Test existing config files
    std::vector<std::string> testFiles;
    testFiles.push_back("configs/minimal.conf");
    testFiles.push_back("configs/basic.conf");
    testFiles.push_back("test_simple.conf");
    testFiles.push_back("test_complex.conf");
    testFiles.push_back("test_error.conf");
    
    int passed = 0;
    int total = testFiles.size();
    
    for (std::vector<std::string>::const_iterator it = testFiles.begin(); it != testFiles.end(); ++it) {
        const std::string& file = *it;
        if (testConfigFile(file)) {
            passed++;
            std::cout << "✓ PASSED" << std::endl;
        } else {
            std::cout << "✗ FAILED" << std::endl;
        }
    }
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << std::endl;
    
    return (passed == total) ? 0 : 1;
}