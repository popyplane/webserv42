/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerStructures.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/20 17:41:50 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/24 23:18:06 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERSTRUCTURES_HPP
# define SERVERSTRUCTURES_HPP

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

#include "../http/HttpRequest.hpp" // for httpmethod enum

// --- Helper Enums / Types ---

// Enum for HTTP Methods, for type safety beyond just strings
// enum HttpMethod {
// 	GET_METHOD,
// 	POST_METHOD,
// 	DELETE_METHOD,
// 	// PUT_METHOD, // creating/updating files
// 	// HEAD_METHOD, // request ressource's header : good practice
// 	// OPTIONS_METHOD, // describing communication options : good practice
// 	UNKNOWN_METHOD
// };

// Enum for log levels
enum LogLevel {
	DEBUG_LOG,
	INFO_LOG,
	WARN_LOG,
	ERROR_LOG,
	CRIT_LOG,
	ALERT_LOG,
	EMERG_LOG,
	DEFAULT_LOG // special case for when no level is specified
};

// --- Location Configuration Structure ---
// Represents the configuration for a single 'location' block
struct LocationConfig {
	// Subject Requirement: "Définir un répertoire ou un fichier à partir duquel le fichier doit être recherché" (root)
	// Justification: The server needs a concrete path to serve files from for this location.
	// Example: root /var/www/html; -> "/var/www/html"
	std::string             root;

	// Subject Requirement: "Définir une liste de méthodes HTTP acceptées pour la route."
	// Justification: Direct access to allowed methods for incoming request validation.
	// Example: allowed_methods GET POST; -> {GET_METHOD, POST_METHOD}
	std::vector<HttpMethod> allowedMethods;

	// Subject Requirement: "Set un fichier par défaut comme réponse si la requête est un répertoire." (index)
	// Justification: The server needs a list of default files to try if a directory is requested.
	// Example: index index.html index.php; -> {"index.html", "index.php"}
	std::vector<std::string> indexFiles;

	// Subject Requirement: "Activer ou désactiver le listing des répertoires." (autoindex)
	// Justification: Boolean flag for directory listing behavior.
	// Example: autoindex on; -> true
	bool                    autoindex;

	// Subject Requirement: "Rendre la route capable d'accepter les fichiers téléversés" (upload_enabled)
	// Justification: Boolean flag to enable/disable file uploads for this path.
	// Example: upload_enabled on; -> true
	bool                    uploadEnabled;

	// Subject Requirement: "configurer où cela doit être enregistré." (upload_store)
	// Justification: The server needs the directory path where uploaded files should be saved.
	// Example: upload_store /var/www/uploads; -> "/var/www/uploads"
	std::string             uploadStore;

	// Subject Requirement: "Exécuter CGI en fonction de certaines extensions de fichier (par exemple.php)."
	// Justification: Maps file extensions to their CGI executable paths.
	// Example: cgi_extension .php; cgi_path /usr/bin/php-cgi; -> {".php": "/usr/bin/php-cgi"}
	std::map<std::string, std::string> cgiExecutables; // Maps extension to path

	// Subject Requirement: "Définir une redirection HTTP." (return)
	// Justification: Stores the redirection status code and the target URL/text.
	// Example: return 301 /new_path; -> {301, "/new_path"}
	int                     returnCode; // 0 if no return, else status code
	std::string             returnUrlOrText;

	// Parser-specific data, crucial for matching logic
	// Justification: The server's request router needs to know the pattern and type
	// to match incoming request URIs.
	std::string             path;       // e.g., "/", "/images/", "= /exact"
	std::string             matchType;  // "" for prefix, "=" for exact, "~" for regex (if you add regex)

	// Nested locations (if your grammar allows and server logic supports)
	// Justification: For hierarchical configuration, allows specific rules for sub-paths.
	std::vector<LocationConfig> nestedLocations;

    // --- NEW: Inherited from server for overriding ---
    // Subject Requirement: "Setup des pages d'erreur par défaut." (can be overridden in locations)
    // Justification: Allows location-specific custom error pages.
    std::map<int, std::string>  errorPages;

    // Subject Requirement: "Limiter la taille du body des clients." (can be overridden in locations)
    // Justification: Allows location-specific client body size limits.
    long                        clientMaxBodySize; // Stored in bytes

	// Constructor to set sensible defaults
	LocationConfig() : root(""), autoindex(false), uploadEnabled(false), uploadStore(""),
					   returnCode(0), path("/"), matchType("") {}
};

// --- Server Configuration Structure ---
// Represents the configuration for a single 'server' block
struct ServerConfig {
	// Subject Requirement: "Choisir le port et l'host de chaque 'serveur'."
	// Justification: The server must bind to specific network interfaces.
	// Example: listen 8080; -> 8080 (port), "0.0.0.0" (host, if not specified)
	std::string                 host;
	int                         port;

	// Subject Requirement: "Setup server_names ou pas."
	// Justification: The server needs to know which hostnames it should respond to.
	// Example: server_name example.com www.example.com; -> {"example.com", "www.example.com"}
	std::vector<std::string>    serverNames;

	// Subject Requirement: "Setup des pages d'erreur par défaut."
	// Justification: Maps HTTP error codes to custom error page paths.
	// Example: error_page 404 /404.html; -> {404: "/404.html"}
	std::map<int, std::string>  errorPages;

	// Subject Requirement: "Limiter la taille du body des clients."
	// Justification: Enforces maximum allowed size for request bodies (e.g., for uploads).
	// Example: client_max_body_size 10m; -> 10 * 1024 * 1024
	long                        clientMaxBodySize; // Stored in bytes

	// Subject Requirement: (Implied by robustness) Error logging path and level
	// Justification: Critical for debugging and monitoring server health.
	// Example: error_log /var/log/webserv_error.log error;
	std::string                 errorLogPath;
	LogLevel                    errorLogLevel;

	// Default configuration inherited by locations if not overridden
	// Justification: Simplifies configuration and matches Nginx behavior.
	std::string                 root;
	std::vector<std::string>    indexFiles;
	bool                        autoindex;

	// Subject Requirement: "Setup des routes avec une ou plusieurs des règles/configurations suivantes"
	// Justification: Contains all the specific path-based configurations.
	std::vector<LocationConfig> locations;

	// Constructor to set sensible defaults
	ServerConfig() : host("0.0.0.0"), port(80), clientMaxBodySize(1048576), // Default 1MB
					 errorLogPath(""), errorLogLevel(DEFAULT_LOG),
					 root(""), autoindex(false) {} // These roots/autoindex will be overridden if set
};

// --- Top-level configuration (list of servers) ---
// Justification: A webserv typically manages multiple independent server blocks.
struct GlobalConfig {
	std::vector<ServerConfig> servers;
	// Potentially global directives if your subject allowed them
	// (e.g., common error_log for all servers if not overridden)
};

// --- Helper functions for parsing string to enum/long ---
// (These would typically be in your ConfigLoader.cpp or a helper utility file)

HttpMethod stringToHttpMethod(const std::string& methodStr);
LogLevel stringToLogLevel(const std::string& levelStr);
long parseSizeToBytes(const std::string& sizeStr);

#endif // SERVER_STRUCTURES_HPP
