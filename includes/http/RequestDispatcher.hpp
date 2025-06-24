/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestDispatcher.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/24 14:45:07 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/24 14:45:33 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_DISPATCHER_HPP
# define REQUEST_DISPATCHER_HPP

#include "../config/ServerStructures.hpp" // For GlobalConfig, ServerConfig, LocationConfig
#include "HttpRequest.hpp"                // For HttpRequest

#include <string>
#include <vector>
#include <map>
#include <utility> // For std::pair

// Forward declaration to avoid circular dependencies if needed elsewhere
class HttpRequest;

/**
 * @brief Represents the result of dispatching an HttpRequest to a configuration.
 * Holds pointers to the matched ServerConfig and LocationConfig.
 * A nullptr indicates no match or a default behavior should be applied.
 */
struct MatchedConfig {
    const ServerConfig* server_config;
    const LocationConfig* location_config; // The *most specific* matched location

    // Constructor to initialize pointers to nullptr
    MatchedConfig() : server_config(NULL), location_config(NULL) {}
};

/**
 * @brief The RequestDispatcher class is responsible for matching an incoming
 * HTTP request to the appropriate server and location configuration
 * based on the loaded GlobalConfig.
 */
class RequestDispatcher {
private:
    const GlobalConfig& _globalConfig; // Reference to the loaded global configuration

    // Helper method to find the best-matching server configuration for a given host and port
    // This involves matching against listen directives and then server_names
    const ServerConfig* findMatchingServer(const HttpRequest& request,
                                           const std::string& clientHost, int clientPort) const;

    // Helper method to find the most specific location configuration within a server block
    const LocationConfig* findMatchingLocation(const HttpRequest& request,
                                               const ServerConfig& serverConfig) const;

    // Helper to get the effective root path for a request
    std::string getEffectiveRoot(const ServerConfig* server, const LocationConfig* location) const;

    // Helper to get the effective client max body size
    long getEffectiveClientMaxBodySize(const ServerConfig* server, const LocationConfig* location) const;

    // Helper to get the effective error pages map
    const std::map<int, std::string>& getEffectiveErrorPages(const ServerConfig* server, const LocationConfig* location) const;


public:
    /**
     * @brief Constructor for the RequestDispatcher.
     * @param globalConfig A constant reference to the loaded global configuration.
     */
    RequestDispatcher(const GlobalConfig& globalConfig);

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
    MatchedConfig dispatch(const HttpRequest& request, const std::string& clientHost, int clientPort) const;
};

#endif // REQUEST_DISPATCHER_HPP
