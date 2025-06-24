/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestDispatcher.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/24 14:46:23 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/24 14:46:42 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/RequestDispatcher.hpp"
#include "../../includes/utils/StringUtils.hpp" // Assuming you have a toLower or similar for host comparison if needed

#include <iostream> // For debugging, remove in final version
#include <limits> // For numeric_limits

// Constructor
RequestDispatcher::RequestDispatcher(const GlobalConfig& globalConfig)
    : _globalConfig(globalConfig) {}

/**
 * @brief Helper to find the best-matching server configuration for a given host and port.
 * Prioritizes by server_name, then the first defined server for a host:port.
 * @param request The parsed HttpRequest.
 * @param clientHost The IP address the client connected to (e.g., "127.0.0.1").
 * @param clientPort The port the client connected to (e.g., 8080).
 * @return Pointer to the matched ServerConfig, or NULL if no server matches.
 */
const ServerConfig* RequestDispatcher::findMatchingServer(const HttpRequest& request,
                                                       const std::string& clientHost, int clientPort) const {
    const ServerConfig* defaultServer = NULL;
    const ServerConfig* nameMatchServer = NULL;

    // Get the Host header from the request
    std::string requestHostHeader = request.getHeader("host");
    // Remove port from Host header if present (e.g., "example.com:8080" -> "example.com")
    size_t colonPos = requestHostHeader.find(':');
    if (colonPos != std::string::npos) {
        requestHostHeader = requestHostHeader.substr(0, colonPos);
    }
    // Convert to lowercase for case-insensitive comparison (HTTP hostnames are case-insensitive)
    StringUtils::toLower(requestHostHeader);


    for (size_t i = 0; i < _globalConfig.servers.size(); ++i) {
        const ServerConfig& currentServer = _globalConfig.servers[i];

        // 1. Check if the server listens on the correct host:port
        // If host is "0.0.0.0" (INADDR_ANY), it matches any clientHost for the given port.
        // Otherwise, clientHost must exactly match the server's host.
        bool hostMatches = (currentServer.host == "0.0.0.0" || currentServer.host == clientHost);
        bool portMatches = (currentServer.port == clientPort);

        if (hostMatches && portMatches) {
            // This server listens on the correct IP:Port
            
            // If this is the first server found for this host:port, it becomes the default.
            if (!defaultServer) {
                defaultServer = &currentServer;
            }

            // 2. Check for server_name match
            for (size_t j = 0; j < currentServer.serverNames.size(); ++j) {
                std::string serverNameLower = currentServer.serverNames[j];
                StringUtils::toLower(serverNameLower); // Convert server_name to lowercase

                if (serverNameLower == requestHostHeader) {
                    // Exact match found! This server is the best candidate.
                    return &currentServer;
                }
            }
        }
    }

    // If no exact server_name match was found, return the first default server
    // that matched the host:port, or NULL if no server matched at all.
    return defaultServer;
}

/**
 * @brief Helper to find the most specific location configuration within a server block.
 * Uses longest prefix matching, with exact matches taking precedence.
 * @param request The parsed HttpRequest.
 * @param serverConfig The matched ServerConfig.
 * @return Pointer to the most specific matched LocationConfig, or NULL if no location matches.
 */
const LocationConfig* RequestDispatcher::findMatchingLocation(const HttpRequest& request,
                                                           const ServerConfig& serverConfig) const {
    const LocationConfig* bestMatch = NULL;
    size_t longestMatchLength = 0;
    bool exactMatchFound = false;

    for (size_t i = 0; i < serverConfig.locations.size(); ++i) {
        const LocationConfig& currentLocation = serverConfig.locations[i];

        // Prioritize exact matches
        if (currentLocation.matchType == "=") {
            if (request.path == currentLocation.path) {
                // If an exact match is found, it immediately takes precedence.
                // Any further matches (even other exact ones) won't be better.
                return &currentLocation;
            }
            continue; // Move to next location if not an exact match for '=' type
        }

        // Standard prefix matching
        if (request.path.rfind(currentLocation.path, 0) == 0) { // Check if currentLocation.path is a prefix of request.path
            if (currentLocation.path.length() > longestMatchLength) {
                longestMatchLength = currentLocation.path.length();
                bestMatch = &currentLocation;
            }
        }
    }

    // After iterating all locations, return the best prefix match found.
    return bestMatch;
}

// Helper to get the effective root path
std::string RequestDispatcher::getEffectiveRoot(const ServerConfig* server, const LocationConfig* location) const {
    if (location && !location->root.empty()) {
        return location->root;
    }
    if (server && !server->root.empty()) {
        return server->root;
    }
    return ""; // Should ideally never be empty in a real config, but handle as default
}

// Helper to get the effective client max body size
long RequestDispatcher::getEffectiveClientMaxBodySize(const ServerConfig* server, const LocationConfig* location) const {
    if (location && location->clientMaxBodySize != 0) { // Assuming 0 means not set/default, adjust if config uses negative for unlimited
        return location->clientMaxBodySize;
    }
    if (server && server->clientMaxBodySize != 0) {
        return server->clientMaxBodySize;
    }
    return std::numeric_limits<long>::max(); // Default to unlimited if neither specify
}

// Helper to get the effective error pages map
const std::map<int, std::string>& RequestDispatcher::getEffectiveErrorPages(const ServerConfig* server, const LocationConfig* location) const {
    if (location && !location->errorPages.empty()) {
        return location->errorPages;
    }
    if (server) { // Server error pages are always the fallback if location doesn't specify
        return server->errorPages;
    }
    // This case should ideally not be reached if a server config is always matched
    static const std::map<int, std::string> emptyMap;
    return emptyMap;
}

/**
 * @brief Dispatches an HTTP request to find the most appropriate server and
 * location configuration.
 * @param request The parsed HttpRequest object.
 * @param clientHost The IP address of the connected client (used for listen matching).
 * @param clientPort The port the client connected to (used for listen matching).
 * @return A MatchedConfig struct containing pointers to the matched ServerConfig
 * and LocationConfig. Pointers will be NULL if no match is found for
 * server or location respectively.
 */
MatchedConfig RequestDispatcher::dispatch(const HttpRequest& request,
                                          const std::string& clientHost, int clientPort) const {
    MatchedConfig result;

    // 1. Find the matching ServerConfig
    result.server_config = findMatchingServer(request, clientHost, clientPort);

    if (result.server_config) {
        // 2. Find the matching LocationConfig within the selected server
        result.location_config = findMatchingLocation(request, *result.server_config);
    }
    // If result.server_config is NULL, result.location_config will remain NULL (default initialized)
    // which correctly signals no server was found.

    return result;
}
