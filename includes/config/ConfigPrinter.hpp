#ifndef CONFIG_PRINTER_HPP
# define CONFIG_PRINTER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "ServerStructures.hpp" // Include your ServerConfig and LocationConfig structs

namespace ConfigPrinter {

    // Forward declarations
    void printLocationConfig(std::ostream& os, const LocationConfig& loc, int indentLevel);
    void printServerConfig(std::ostream& os, const ServerConfig& server, int indentLevel);
    
    // Main function to print all loaded server configurations
    void printConfig(std::ostream& os, const std::vector<ServerConfig>& servers);

    // Helper for indentation
    std::string getIndent(int level);

} // namespace ConfigPrinter

#endif // CONFIG_PRINTER_HPP
