/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequestHandler.hpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/25 09:51:37 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 14:45:15 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_HANDLER_HPP
# define HTTP_REQUEST_HANDLER_HPP

#include "HttpRequest.hpp"       // For HttpRequest
#include "HttpResponse.hpp"      // For HttpResponse
#include "RequestDispatcher.hpp" // For MatchedConfig (which contains ServerConfig/LocationConfig)
#include "../utils/StringUtils.hpp" // For string utility functions (e.g., path joining)

#include <string>
#include <vector>
#include <map>
#include <sys/stat.h> // For stat() function (checking file/directory existence, mkdir)
#include <fstream>    // For file reading/writing
#include <dirent.h>   // For directory listing (opendir, readdir)
#include <unistd.h>   // For access()
#include <limits>     // For numeric_limits


/**
 * @brief The HttpRequestHandler class processes an HttpRequest based on the
 * matched server and location configurations, and generates an HttpResponse.
 */
class HttpRequestHandler {
public:
    /**
     * @brief Constructor.
     */
    HttpRequestHandler();

    /**
     * @brief Destructor.
     */
    ~HttpRequestHandler();

    /**
     * @brief Handles an incoming HTTP request and generates a corresponding response.
     * This is the main entry point for request processing after dispatching.
     * @param request The parsed HttpRequest.
     * @param matchedConfig The configuration (server and location) determined by the dispatcher.
     * @return An HttpResponse object ready to be sent to the client.
     */
    HttpResponse handleRequest(const HttpRequest& request, const MatchedConfig& matchedConfig);

private:
    // --- Helper Methods for Response Generation ---

    /**
     * @brief Generates an error response based on a status code and optional custom page.
     * @param statusCode The HTTP status code (e.g., 404, 500).
     * @param serverConfig The ServerConfig to check for custom error pages.
     * @param locationConfig The LocationConfig to check for custom error pages (prioritized).
     * @return An HttpResponse representing the error.
     */
    HttpResponse _generateErrorResponse(int statusCode,
                                       const ServerConfig* serverConfig,
                                       const LocationConfig* locationConfig);

    /**
     * @brief Resolves the full file system path for a given URI based on root directives.
     * @param uriPath The path from the HTTP request (e.g., "/images/logo.png").
     * @param serverConfig The matched ServerConfig.
     * @param locationConfig The matched LocationConfig (if any).
     * @return The absolute file system path (e.g., "/var/www/html/images/logo.png").
     */
    std::string _resolvePath(const std::string& uriPath,
                            const ServerConfig* serverConfig,
                            const LocationConfig* locationConfig) const;

    /**
     * @brief Handles a GET request to serve static content or directory listings.
     * @param request The HttpRequest.
     * @param serverConfig The matched ServerConfig.
     * @param locationConfig The matched LocationConfig.
     * @return An HttpResponse with the file content, or an error response.
     */
    HttpResponse _handleGet(const HttpRequest& request,
                           const ServerConfig* serverConfig,
                           const LocationConfig* locationConfig);

    /**
     * @brief Handles a POST request, primarily for file uploads.
     * @param request The HttpRequest (containing body data).
     * @param serverConfig The matched ServerConfig.
     * @param locationConfig The matched LocationConfig.
     * @return An HttpResponse indicating success (201 Created) or an error.
     */
    HttpResponse _handlePost(const HttpRequest& request,
                            const ServerConfig* serverConfig,
                            const LocationConfig* locationConfig);

    /**
     * @brief Handles a DELETE request to remove a resource.
     * @param request The HttpRequest.
     * @param serverConfig The matched ServerConfig.
     * @param locationConfig The matched LocationConfig.
     * @return An HttpResponse indicating success (204 No Content) or an error.
     */
    HttpResponse _handleDelete(const HttpRequest& request,
                              const ServerConfig* serverConfig,
                              const LocationConfig* locationConfig);


    /**
     * @brief Generates an HTML directory listing (for autoindex).
     * @param directoryPath The absolute path to the directory.
     * @param uriPath The URI path for the directory (for links).
     * @return A string containing the HTML listing.
     */
    std::string _generateAutoindexPage(const std::string& directoryPath, const std::string& uriPath) const;

    // --- Utility Methods for File System & Config Access ---

    // Gets the effective root directory for a request (location root takes precedence)
    std::string _getEffectiveRoot(const ServerConfig* server, const LocationConfig* location) const;
    
    // Gets the effective error pages map (location error_page takes precedence)
    const std::map<int, std::string>& _getEffectiveErrorPages(const ServerConfig* server, const LocationConfig* location) const;

    // NEW: Gets the effective upload store path (location upload_store takes precedence)
    std::string _getEffectiveUploadStore(const ServerConfig* server, const LocationConfig* location) const;

    // NEW: Gets the effective client max body size (location client_max_body_size takes precedence)
    long _getEffectiveClientMaxBodySize(const ServerConfig* server, const LocationConfig* location) const;


    // Determines the MIME type based on file extension
    std::string _getMimeType(const std::string& filePath) const;

    // Checks if a path points to a regular file
    bool _isRegularFile(const std::string& path) const;

    // Checks if a path points to a directory
    bool _isDirectory(const std::string& path) const;

    // Checks if a file exists
    bool _fileExists(const std::string& path) const;

    // Checks if a file/directory has read permissions
    bool _canRead(const std::string& path) const;

    // NEW: Checks if a directory has write permissions
    bool _canWrite(const std::string& path) const;
};

#endif // HTTP_REQUEST_HANDLER_HPP
