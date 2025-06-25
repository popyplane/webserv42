/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestDispatcher.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/24 14:46:23 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/24 17:17:53 by baptistevie      ###   ########.fr       */
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
const ServerConfig* RequestDispatcher::findMatchingServer(const HttpRequest& request, const std::string& clientHost, int clientPort) const {
	const ServerConfig* defaultServer = NULL; // Stores the 1st server matching host:port (the default)
	// const ServerConfig* nameMatchServer = NULL; // unused

	// Get the Host header from the request
	std::string requestHostHeader = request.getHeader("host");

	// Remove port from Host header if present (e.g., "example.com:8080" -> "example.com")
	size_t colonPos = requestHostHeader.find(':');
	if (colonPos != std::string::npos) {
		requestHostHeader = requestHostHeader.substr(0, colonPos);
	}
	// Convert to lowercase for case-insensitive comparison (HTTP hostnames are case-insensitive)
	StringUtils::toLower(requestHostHeader);

	// loop through all servers
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
 * @brief Finds the most specific location configuration within a server block.
 *
 * This function implements the routing logic for HTTP requests based on the URI path.
 * It uses the "longest prefix matching" rule to find the location block whose
 * path most specifically matches the beginning of the request's URI path.
 *
 * @param request The parsed HttpRequest object, containing the URI path to match.
 * @param serverConfig The ServerConfig object within which to search for locations.
 * @return A constant pointer to the most specific matching LocationConfig.
 * Returns NULL if no location block (including the root "/") matches the request path.
 */
const LocationConfig* RequestDispatcher::findMatchingLocation(const HttpRequest& request, const ServerConfig& serverConfig) const {
    const LocationConfig* bestMatch = NULL; // Pointer to the best matching location found so far.
    size_t longestMatchLength = 0;          // Tracks the length of the longest matching path prefix.

    // Iterate through all location blocks configured within the given server.
    for (size_t i = 0; i < serverConfig.locations.size(); ++i) {
        const LocationConfig& currentLocation = serverConfig.locations[i];

        // Check if the current location's path is a prefix of the request's URI path.
        // `request.path.rfind(currentLocation.path, 0) == 0` checks if currentLocation.path
        // is found at the very beginning (index 0) of request.path.
        if (request.path.rfind(currentLocation.path, 0) == 0) {
            // If this location matches and its path is longer (more specific)
            // than any previous match, update `bestMatch`.
            if (currentLocation.path.length() > longestMatchLength) {
                longestMatchLength = currentLocation.path.length();
                bestMatch = &currentLocation;
            }
        }
    }

    // After checking all locations, return the location that had the longest matching prefix.
    // If no location matched (e.g., if request.path was not prefixed by any location,
    // and no '/' root location exists), bestMatch will remain NULL.
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
MatchedConfig RequestDispatcher::dispatch(const HttpRequest& request, const std::string& clientHost, int clientPort) const {
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
