/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigLoader.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/20 17:41:45 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/20 17:43:33 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGLOADER_HPP
# define CONFIGLOADER_HPP

#include <vector>
#include <string>
#include <stdexcept> // For std::runtime_error
#include <sstream>   // For string stream operations
#include <algorithm> // For std::transform, std::tolower
#include <limits>    // For std::numeric_limits

#include "ASTnode.hpp"          // To read the AST
#include "ServerStructures.hpp" // To build the target configuration objects

class ConfigLoader {
public:
    ConfigLoader();
    ~ConfigLoader();

    // Main entry point: takes the top-level AST nodes and returns a vector of ServerConfig
    std::vector<ServerConfig> loadConfig(const std::vector<ASTnode*>& astNodes);

private:
    // Helper to parse a single server block from its AST node
    ServerConfig    parseServerBlock(const BlockNode* serverBlockNode);

    // Helper to parse a single location block from its AST node
    // Takes a reference to the parent ServerConfig for default inheritance
    LocationConfig  parseLocationBlock(const BlockNode* locationBlockNode, const ServerConfig& parentServerDefaults);

    // Helper to process a single directive and apply its value to the correct config object
    // This is where string-to-type conversion and basic validation happen for each directive.
    void            processDirective(const DirectiveNode* directive, ServerConfig& serverConfig);
    void            processDirective(const DirectiveNode* directive, LocationConfig& locationConfig);

    // --- Utility functions for type conversion and semantic validation ---

    // Converts string (e.g., "GET") to HttpMethod enum
    HttpMethod      stringToHttpMethod(const std::string& methodStr) const;

    // Converts string (e.g., "error") to LogLevel enum
    LogLevel        stringToLogLevel(const std::string& levelStr) const;

    // Parses size strings (e.g., "10m", "512k") into long bytes
    long            parseSizeToBytes(const std::string& sizeStr) const;

    // Helper to handle errors specific to configuration loading
    void            error(const std::string& msg, int line, int col) const;
};

// Custom exception for configuration loading errors
class ConfigLoadError : public std::runtime_error {
private:
    int _line;
    int _column;
public:
    ConfigLoadError(const std::string& msg, int line = 0, int column = 0)
        : std::runtime_error(msg), _line(line), _column(column) {}
    int getLine() const { return _line; }
    int getColumn() const { return _column; }
};

#endif // CONFIG_LOADER_HPP
