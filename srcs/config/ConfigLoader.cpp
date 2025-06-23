/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigLoader.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/23 11:22:17 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/23 11:27:11 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/config/ConfigLoader.hpp"

// --- ConfigLoader Constructor & Destructor ---

ConfigLoader::ConfigLoader() {}

ConfigLoader::~ConfigLoader() {}

// --- Public Entry Point for Configuration Loading ---

/**
 * @brief Main function to load the entire server configuration from the AST.
 * @param astNodes A vector of ASTnode pointers, expected to contain only BlockNode representing 'server' blocks.
 * @return A vector of ServerConfig objects, each representing a fully loaded server configuration.
 * @throws ConfigLoadError if any semantic errors are found in the configuration.
 */
std::vector<ServerConfig>   ConfigLoader::loadConfig(const std::vector<ASTnode *> & astNodes)
{
	std::vector<ServerConfig>   loadedServers;

	for (size_t i = 0; i < astNodes.size(); ++i) {
		ASTnode * node = astNodes[i];
		BlockNode * serverBlockNode = dynamic_cast<BlockNode *>(node);;

		if (serverBlockNode) { // check if dynamic cast succeed
			if (serverBlockNode->name == "server") { // check if this is a server block node
				loadedServers.push_back(parseServerBlock(serverBlockNode));
			} else { // if it is location block at top level
				error("Unexpected block type '" + serverBlockNode->name + "' at top level. Expected 'server' block.",
					  serverBlockNode->line, serverBlockNode->column);
			}
		} else {
			DirectiveNode* directiveNode = dynamic_cast<DirectiveNode *>(node);

			if (directiveNode) { // check if it's a wrong directive node
				 error("Unexpected directive '" + directiveNode->name + "' at top level. Expected 'server' block.",
					   directiveNode->line, directiveNode->column);
			} else { // should never happen
				 error("Unknown AST node type encountered at top level.", node->line, node->column);
			}
		}
	}
	// For simplicity, a basic check: ensure at least one server is defined if the config is not empty.
	if (loadedServers.empty() && !astNodes.empty()) {
	// This case should ideally be caught earlier if there are non-server blocks,
	// but it's a final safeguard.
		error("No valid server blocks found in configuration.", 0, 0); // Line/col 0 for general error
	}
	return loadedServers;
}

// --- Private Helper Functions for Parsing Blocks ---

/**
 * @brief Parses a single 'server' block from its AST node into a ServerConfig object.
 * @param serverBlockNode The BlockNode representing the 'server' block.
 * @return A fully populated ServerConfig object.
 * @throws ConfigLoadError if unexpected children or semantic errors are found.
 */
ServerConfig    ConfigLoader::parseServerBlock(const BlockNode * serverBlockNode)
{
	ServerConfig serverConf;

	// Set sensible default values for the server config.
	// These defaults will be overridden if explicit directives are found in the block.
	// Note: clientMaxBodySize default 1MB (1 * 1024 * 1024) is set in ServerConfig constructor.
	// root and indexFiles are empty by default in ServerConfig struct.

	for (size_t i = 0; i < serverBlockNode->children.size(); ++i) {
		ASTnode * childNode = serverBlockNode->children[i];
		DirectiveNode * directive = dynamic_cast<DirectiveNode *>(childNode);
		BlockNode * nestedBlock = dynamic_cast<BlockNode *>(childNode);

		if (directive) {
			processDirective(directive, serverConf);
		} else if (nestedBlock && nestedBlock->name == "location") {
			// Call the parseLocationBlock overload that takes a ServerConfig as parent
			serverConf.locations.push_back(parseLocationBlock(nestedBlock, serverConf));
		} else {
			error("Unexpected child node in server block. Expected a directive or 'location' block.",
				  childNode->line, childNode->column);
		}
	}
	// --- Server-specific Semantic Validations after all directives are processed ---
	// Example: Ensure a 'listen' directive was provided.
	if (serverConf.port == 0) { // Default port might be 0, check if it was set explicitly
		error("Server block is missing a 'listen' directive or it's invalid.",
			  serverBlockNode->line, serverBlockNode->column); // Use block start for general error
	}
	// Example: Ensure root is defined if not set later
	if (serverConf.root.empty() && serverConf.locations.empty()) {
		// If there's no root AND no locations, this server can't serve anything.
		error("Server block has no 'root' directive and no 'location' blocks defined. Cannot serve content.",
			  serverBlockNode->line, serverBlockNode->column);
	}
	return (serverConf);
}

/**
 * @brief Parses a top-level 'location' block from its AST node into a LocationConfig object.
 * Inherits defaults from a parent ServerConfig.
 * @param locationBlockNode The BlockNode representing the 'location' block.
 * @param parentServerDefaults The ServerConfig of the parent server, used for inheriting default values.
 * @return A fully populated LocationConfig object.
 * @throws ConfigLoadError if unexpected children or semantic errors are found.
 */
LocationConfig    ConfigLoader::parseLocationBlock(const BlockNode * locationBlockNode, const ServerConfig & parentServerDefaults)
{
	LocationConfig locationConf;

	// --- Step 1: Initialize LocationConfig with inherited defaults from parentServerDefaults ---
	locationConf.root = parentServerDefaults.root;
	locationConf.indexFiles = parentServerDefaults.indexFiles;
	locationConf.autoindex = parentServerDefaults.autoindex;
	locationConf.errorPages = parentServerDefaults.errorPages;
	locationConf.clientMaxBodySize = parentServerDefaults.clientMaxBodySize;

	// --- Step 2: Load the location block's own arguments (path and matchType) ---
	if (locationBlockNode->args.empty()) {
		error("Location block requires at least a path argument.",
			  locationBlockNode->line, locationBlockNode->column);
	} else if (locationBlockNode->args.size() == 1) {
		locationConf.path = locationBlockNode->args[0];
		locationConf.matchType = ""; // Default to prefix match
	} else if (locationBlockNode->args.size() == 2) {
		locationConf.matchType = locationBlockNode->args[0];
		locationConf.path = locationBlockNode->args[1];
		if (locationConf.matchType != "=" &&
			locationConf.matchType != "~" &&
			locationConf.matchType != "~*" &&
			locationConf.matchType != "^~") {
			error("Invalid location match type '" + locationConf.matchType + "'. Expected '=', '~', '~*', or '^~'.",
				  locationBlockNode->line, locationBlockNode->column);
		}
	} else {
		error("Location block has too many arguments. Expected a path or a modifier and a path.",
			  locationBlockNode->line, locationBlockNode->column);
	}

	// --- Step 3: Iterate and process child directives and nested location blocks ---
	for (size_t i = 0; i < locationBlockNode->children.size(); ++i) {
		ASTnode * childNode = locationBlockNode->children[i];
		DirectiveNode * directive = dynamic_cast<DirectiveNode *>(childNode);
		BlockNode * nestedBlock = dynamic_cast<BlockNode *>(childNode);

		if (directive) {
			processDirective(directive, locationConf);
		} else if (nestedBlock && nestedBlock->name == "location") {
			// HERE: Recursively call the NEW overload that takes a LocationConfig as parent
			locationConf.nestedLocations.push_back(parseLocationBlock(nestedBlock, locationConf));
		} else {
			error("Unexpected child node in location block. Expected a directive or a nested 'location' block.",
				  childNode->line, childNode->column);
		}
	}

	// --- Step 4: Location-specific Semantic Validations after all directives are processed ---
	if (locationConf.root.empty()) {
		error("Location block is missing a 'root' directive or it's not inherited.",
			  locationBlockNode->line, locationBlockNode->column);
	}
	if (locationConf.uploadEnabled && locationConf.uploadStore.empty()) {
		error("Uploads are enabled but 'upload_store' directive is missing or invalid.",
			  locationBlockNode->line, locationBlockNode->column);
	}
	if (!locationConf.cgiExecutables.empty()) {
		bool cgi_path_missing = false;
		std::map<std::string, std::string>::const_iterator it;
		for (it = locationConf.cgiExecutables.begin(); it != locationConf.cgiExecutables.end(); ++it) {
			if (it->second.empty()) {
				cgi_path_missing = true;
				break;
			}
		}
		if (cgi_path_missing) {
			error("CGI extensions defined but corresponding 'cgi_path' is missing or invalid.",
				  locationBlockNode->line, locationBlockNode->column);
		}
	}
	return (locationConf);
}

/**
 * @brief Parses a nested 'location' block from its AST node into a LocationConfig object.
 * Inherits defaults from a parent LocationConfig.
 * @param locationBlockNode The BlockNode representing the 'location' block.
 * @param parentLocationDefaults The LocationConfig of the parent location, used for inheriting default values.
 * @return A fully populated LocationConfig object.
 * @throws ConfigLoadError if unexpected children or semantic errors are found.
 */
LocationConfig    ConfigLoader::parseLocationBlock(const BlockNode * locationBlockNode, const LocationConfig & parentLocationDefaults)
{
	LocationConfig locationConf;

	// --- Step 1: Initialize LocationConfig with inherited defaults from parentLocationDefaults ---
	// A nested location inherits from its immediate parent location.
	locationConf.root = parentLocationDefaults.root;
	locationConf.indexFiles = parentLocationDefaults.indexFiles;
	locationConf.autoindex = parentLocationDefaults.autoindex;
	locationConf.errorPages = parentLocationDefaults.errorPages;
	locationConf.clientMaxBodySize = parentLocationDefaults.clientMaxBodySize;
	locationConf.allowedMethods = parentLocationDefaults.allowedMethods; // Location-specific methods also inherit.
	locationConf.uploadEnabled = parentLocationDefaults.uploadEnabled;
	locationConf.uploadStore = parentLocationDefaults.uploadStore;
	locationConf.cgiExecutables = parentLocationDefaults.cgiExecutables; // Inherit CGI settings
	locationConf.returnCode = parentLocationDefaults.returnCode;
	locationConf.returnUrlOrText = parentLocationDefaults.returnUrlOrText;

	// --- Step 2: Load the location block's own arguments (path and matchType) ---
	// This logic is identical to the other overload as it's about the block's own definition.
	if (locationBlockNode->args.empty()) {
		error("Location block requires at least a path argument.",
			  locationBlockNode->line, locationBlockNode->column);
	} else if (locationBlockNode->args.size() == 1) {
		locationConf.path = locationBlockNode->args[0];
		locationConf.matchType = ""; // Default to prefix match
	} else if (locationBlockNode->args.size() == 2) {
		locationConf.matchType = locationBlockNode->args[0];
		locationConf.path = locationBlockNode->args[1];
		if (locationConf.matchType != "=" &&
			locationConf.matchType != "~" &&
			locationConf.matchType != "~*" &&
			locationConf.matchType != "^~") {
			error("Invalid location match type '" + locationConf.matchType + "'. Expected '=', '~', '~*', or '^~'.",
				  locationBlockNode->line, locationBlockNode->column);
		}
	} else {
		error("Location block has too many arguments. Expected a path or a modifier and a path.",
			  locationBlockNode->line, locationBlockNode->column);
	}

	// --- Step 3: Iterate and process child directives and further nested location blocks ---
	for (size_t i = 0; i < locationBlockNode->children.size(); ++i) {
		ASTnode * childNode = locationBlockNode->children[i];
		DirectiveNode * directive = dynamic_cast<DirectiveNode *>(childNode);
		BlockNode * nestedBlock = dynamic_cast<BlockNode *>(childNode);

		if (directive) {
			processDirective(directive, locationConf); // This call overrides inherited values
		} else if (nestedBlock && nestedBlock->name == "location") {
			// HERE: Recursively call THIS OVERLOAD for deeper nested locations
			locationConf.nestedLocations.push_back(parseLocationBlock(nestedBlock, locationConf));
		} else {
			error("Unexpected child node in location block. Expected a directive or a nested 'location' block.",
				  childNode->line, childNode->column);
		}
	}

	// --- Location-specific Semantic Validations after all directives are processed ---
	// These validations are identical across both overloads as they pertain to the final state of LocationConfig.
	if (locationConf.root.empty()) {
		error("Location block is missing a 'root' directive or it's not inherited.",
			  locationBlockNode->line, locationBlockNode->column);
	}
	if (locationConf.uploadEnabled && locationConf.uploadStore.empty()) {
		error("Uploads are enabled but 'upload_store' directive is missing or invalid.",
			  locationBlockNode->line, locationBlockNode->column);
	}
	if (!locationConf.cgiExecutables.empty()) {
		bool cgi_path_missing = false;
		std::map<std::string, std::string>::const_iterator it;
		for (it = locationConf.cgiExecutables.begin(); it != locationConf.cgiExecutables.end(); ++it) {
			if (it->second.empty()) {
				cgi_path_missing = true;
				break;
			}
		}
		if (cgi_path_missing) {
			error("CGI extensions defined but corresponding 'cgi_path' is missing or invalid.",
				  locationBlockNode->line, locationBlockNode->column);
		}
	}
	return (locationConf);
}


// --- Private Dispatcher Functions for Directives ---

/**
 * @brief Dispatches a DirectiveNode to the appropriate handler for ServerConfig.
 * @param directive The DirectiveNode to process.
 * @param serverConfig The ServerConfig object to modify.
 * @throws ConfigLoadError if the directive is not recognized in this context.
 */
void ConfigLoader::processDirective(const DirectiveNode* directive, ServerConfig& serverConfig) {
	const std::string& name = directive->name;

	// Server-specific directives
	if (name == "listen") {
		handleListenDirective(directive, serverConfig);
	} else if (name == "server_name") {
		handleServerNameDirective(directive, serverConfig);
	} else if (name == "error_log") {
		handleErrorLogDirective(directive, serverConfig);
	} 
	// Directives common to both Server and Location contexts
	else if (name == "root") {
		handleRootDirective(directive, serverConfig);
	} else if (name == "index") {
		handleIndexDirective(directive, serverConfig);
	} else if (name == "autoindex") {
		handleAutoindexDirective(directive, serverConfig);
	} else if (name == "error_page") {
		handleErrorPageDirective(directive, serverConfig);
	} else if (name == "client_max_body_size") {
		handleClientMaxBodySizeDirective(directive, serverConfig);
	}
	// If a directive name is recognized by the parser but not handled here, or
	// if it's a directive specifically for location blocks, it's an error.
	else {
		error("Unexpected directive '" + name + "' in server context.",
			  directive->line, directive->column);
	}
}

/**
 * @brief Dispatches a DirectiveNode to the appropriate handler for LocationConfig.
 * @param directive The DirectiveNode to process.
 * @param locationConfig The LocationConfig object to modify.
 * @throws ConfigLoadError if the directive is not recognized in this context.
 */
void ConfigLoader::processDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::string& name = directive->name;

	// Directives common to both Server and Location contexts
	if (name == "root") {
		handleRootDirective(directive, locationConfig);
	} else if (name == "index") {
		handleIndexDirective(directive, locationConfig);
	} else if (name == "autoindex") {
		handleAutoindexDirective(directive, locationConfig);
	} else if (name == "error_page") {
		handleErrorPageDirective(directive, locationConfig);
	} else if (name == "client_max_body_size") {
		handleClientMaxBodySizeDirective(directive, locationConfig);
	}
	// Location-specific directives
	else if (name == "allowed_methods") {
		handleAllowedMethodsDirective(directive, locationConfig);
	} else if (name == "upload_enabled") {
		handleUploadEnabledDirective(directive, locationConfig);
	} else if (name == "upload_store") {
		handleUploadStoreDirective(directive, locationConfig);
	} else if (name == "cgi_extension") {
		handleCgiExtensionDirective(directive, locationConfig);
	} else if (name == "cgi_path") {
		handleCgiPathDirective(directive, locationConfig);
	} else if (name == "return") {
		handleReturnDirective(directive, locationConfig);
	}
	// If a directive name is recognized by the parser but not handled here, or
	// if it's a directive specifically for server blocks, it's an error.
	else {
		error("Unexpected directive '" + name + "' in location context.",
			  directive->line, directive->column);
	}
}


// --- Private Helper Functions for Directive Processing (Detailed Implementations) ---

// --- Server-Specific Handlers ---

/**
 * @brief Handles the 'listen' directive for a ServerConfig.
 * @param directive The 'listen' DirectiveNode.
 * @param serverConfig The ServerConfig object to update.
 * @throws ConfigLoadError if arguments are invalid or out of range.
 */
void ConfigLoader::handleListenDirective(const DirectiveNode* directive, ServerConfig& serverConfig) {
	const std::vector<std::string>& args = directive->args;
	
	// Validate argument count: 'listen' must have exactly one argument.
	if (args.size() != 1) {
		error("Directive 'listen' requires exactly one argument (port or IP:port).",
			  directive->line, directive->column);
	}

	const std::string& listenArg = args[0];
	size_t colon_pos = listenArg.find(':');

	if (colon_pos != std::string::npos) {
		// Case: IP:Port (e.g., "127.0.0.1:8080")
		std::string ip_str = listenArg.substr(0, colon_pos);
		std::string port_str = listenArg.substr(colon_pos + 1);

		// Basic IP string validation: cannot be empty.
		// Full IP address validation (e.g., checking octet ranges) is more complex and typically
		// done by OS networking functions later, or with regular expressions (not C++98 standard).
		if (ip_str.empty()) {
			error("Listen directive: IP address part cannot be empty in IP:Port format.",
				  directive->line, directive->column);
		}
		serverConfig.host = ip_str; // Assign the parsed IP string.

		// Port string validation and conversion.
		try {
			if (port_str.empty() || !StringUtils::isDigits(port_str)) {
				throw std::invalid_argument("Port must be a number.");
			}
			long port_long = StringUtils::stringToLong(port_str); // Convert to long to safely check range.
			if (port_long < 1 || port_long > 65535) { // Standard TCP/UDP port range.
				throw std::out_of_range("Port number out of valid range (1-65535).");
			}
			serverConfig.port = static_cast<int>(port_long); // Safe cast after range check.
		} catch (const std::invalid_argument& e) {
			error("Listen directive: Invalid port format in IP:Port format. " + std::string(e.what()),
				  directive->line, directive->column);
		} catch (const std::out_of_range& e) {
			error("Listen directive: " + std::string(e.what()) + " in IP:Port format.",
				  directive->line, directive->column);
		}

	} else {
		// Case: Just Port (e.g., "8080")
		// In this case, host defaults to "0.0.0.0" (any available IP address).
		
		// Port string validation and conversion.
		try {
			if (listenArg.empty() || !StringUtils::isDigits(listenArg)) {
				throw std::invalid_argument("Argument must be a port number.");
			}
			long port_long = StringUtils::stringToLong(listenArg);
			if (port_long < 1 || port_long > 65535) {
				throw std::out_of_range("Port number out of valid range (1-65535).");
			}
			serverConfig.host = "0.0.0.0"; // Default host to listen on all interfaces.
			serverConfig.port = static_cast<int>(port_long);
		} catch (const std::invalid_argument& e) {
			error("Listen directive: Invalid port format. " + std::string(e.what()),
				  directive->line, directive->column);
		} catch (const std::out_of_range& e) {
			error("Listen directive: " + std::string(e.what()),
				  directive->line, directive->column);
		}
	}
}

/**
 * @brief Handles the 'server_name' directive for a ServerConfig.
 * @param directive The 'server_name' DirectiveNode.
 * @param serverConfig The ServerConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleServerNameDirective(const DirectiveNode* directive, ServerConfig& serverConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: 'server_name' requires at least one hostname.
	if (args.empty()) {
		error("Directive 'server_name' requires at least one argument (hostname).",
			  directive->line, directive->column);
	}

	// Assign all arguments as server names. Basic hostname validation is done during lexing
	// (identifier vs. string), but more specific checks (e.g., valid characters, no leading dot)
	// could be added here if desired.
	serverConfig.serverNames = args;
}

/**
 * @brief Handles the 'error_log' directive for a ServerConfig.
 * @param directive The 'error_log' DirectiveNode.
 * @param serverConfig The ServerConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleErrorLogDirective(const DirectiveNode* directive, ServerConfig& serverConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: 'error_log' takes 1 (path) or 2 (path + level) arguments.
	if (args.empty() || args.size() > 2) {
		error("Directive 'error_log' requires one or two arguments: a file path and optional log level.",
			  directive->line, directive->column);
	}

	// First argument is the log file path.
	serverConfig.errorLogPath = args[0];
	// Basic validation: Path should not be empty.
	if (serverConfig.errorLogPath.empty()) {
		error("Error log path cannot be empty.", directive->line, directive->column);
	}

	// If a second argument is provided, it's the log level.
	if (args.size() == 2) {
		try {
			serverConfig.errorLogLevel = stringToLogLevel(args[1]); // Use helper for conversion and validation.
		} catch (const std::invalid_argument& e) {
			error("Error log level invalid. " + std::string(e.what()),
				  directive->line, directive->column);
		}
	}
	// If only one argument, errorLogLevel remains its default value (DEFAULT_LOG from constructor).
}

// --- Common Directives (Overloaded Handlers) ---

/**
 * @brief Handles the 'root' directive for a ServerConfig.
 * @param directive The 'root' DirectiveNode.
 * @param serverConfig The ServerConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleRootDirective(const DirectiveNode* directive, ServerConfig& serverConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: 'root' requires exactly one argument.
	if (args.size() != 1) {
		error("Directive 'root' requires exactly one argument (directory path).",
			  directive->line, directive->column);
	}
	// Basic path validation: Should not be empty.
	if (args[0].empty()) {
		error("Root path cannot be empty.", directive->line, directive->column);
	}
	serverConfig.root = args[0]; // Assign the root path.
}

/**
 * @brief Handles the 'root' directive for a LocationConfig.
 * @param directive The 'root' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleRootDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	if (args.size() != 1) {
		error("Directive 'root' requires exactly one argument (directory path).",
			  directive->line, directive->column);
	}
	if (args[0].empty()) {
		error("Root path cannot be empty.", directive->line, directive->column);
	}
	locationConfig.root = args[0]; // Assign the root path. This overrides inherited value.
}

/**
 * @brief Handles the 'index' directive for a ServerConfig.
 * @param directive The 'index' DirectiveNode.
 * @param serverConfig The ServerConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleIndexDirective(const DirectiveNode* directive, ServerConfig& serverConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: 'index' requires at least one filename.
	if (args.empty()) {
		error("Directive 'index' requires at least one argument (filename).",
			  directive->line, directive->column);
	}
	// Assign all arguments as index files. This replaces any previous list of index files.
	serverConfig.indexFiles = args;
}

/**
 * @brief Handles the 'index' directive for a LocationConfig.
 * @param directive The 'index' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleIndexDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	if (args.empty()) {
		error("Directive 'index' requires at least one argument (filename).",
			  directive->line, directive->column);
	}
	locationConfig.indexFiles = args; // This overrides inherited index files.
}

/**
 * @brief Handles the 'autoindex' directive for a ServerConfig.
 * @param directive The 'autoindex' DirectiveNode.
 * @param serverConfig The ServerConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleAutoindexDirective(const DirectiveNode* directive, ServerConfig& serverConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: 'autoindex' requires exactly one argument.
	if (args.size() != 1) {
		error("Directive 'autoindex' requires exactly one argument ('on' or 'off').",
			  directive->line, directive->column);
	}
	// Validate argument value: Must be "on" or "off".
	if (args[0] == "on") {
		serverConfig.autoindex = true;
	} else if (args[0] == "off") {
		serverConfig.autoindex = false;
	} else {
		error("Argument for 'autoindex' must be 'on' or 'off', but got '" + args[0] + "'.",
			  directive->line, directive->column);
	}
}

/**
 * @brief Handles the 'autoindex' directive for a LocationConfig.
 * @param directive The 'autoindex' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleAutoindexDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	if (args.size() != 1) {
		error("Directive 'autoindex' requires exactly one argument ('on' or 'off').",
			  directive->line, directive->column);
	}
	if (args[0] == "on") {
		locationConfig.autoindex = true; // Overrides inherited autoindex setting.
	} else if (args[0] == "off") {
		locationConfig.autoindex = false;
	} else {
		error("Argument for 'autoindex' must be 'on' or 'off', but got '" + args[0] + "'.",
			  directive->line, directive->column);
	}
}

/**
 * @brief Handles the 'error_page' directive for a ServerConfig.
 * @param directive The 'error_page' DirectiveNode.
 * @param serverConfig The ServerConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleErrorPageDirective(const DirectiveNode* directive, ServerConfig& serverConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: at least one code + one URI.
	if (args.size() < 2) {
		error("Directive 'error_page' requires at least two arguments: one or more error codes followed by a URI.",
			  directive->line, directive->column);
	}

	// The last argument is the URI for the error page.
	const std::string& uri = args[args.size() - 1];
	if (uri.empty() || uri[0] != '/') { // Basic URI path validation: must be non-empty and absolute.
		error("Error page URI must be an absolute path (e.g., '/error.html').",
			  directive->line, directive->column);
	}

	// Iterate through the error codes (all arguments except the last one).
	for (size_t i = 0; i < args.size() - 1; ++i) {
		try {
			if (!StringUtils::isDigits(args[i])) {
				throw std::invalid_argument("Error code must be a number.");
			}
			long code_long = StringUtils::stringToLong(args[i]);
			// Validate HTTP status code range (100-599).
			if (code_long < 100 || code_long > 599) {
				throw std::out_of_range("Error code out of valid HTTP status code range (100-599).");
			}
			// Add or update the mapping in the errorPages map.
			serverConfig.errorPages[static_cast<int>(code_long)] = uri;
		} catch (const std::invalid_argument& e) {
			error("Error code for 'error_page' invalid: " + std::string(e.what()),
				  directive->line, directive->column);
		} catch (const std::out_of_range& e) {
			error("Error code for 'error_page' " + std::string(e.what()),
				  directive->line, directive->column);
		}
	}
}

/**
 * @brief Handles the 'error_page' directive for a LocationConfig.
 * @param directive The 'error_page' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleErrorPageDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	if (args.size() < 2) {
		error("Directive 'error_page' requires at least two arguments: one or more error codes followed by a URI.",
			  directive->line, directive->column);
	}

	const std::string& uri = args[args.size() - 1];
	if (uri.empty() || uri[0] != '/') {
		error("Error page URI must be an absolute path (e.g., '/error.html').",
			  directive->line, directive->column);
	}

	for (size_t i = 0; i < args.size() - 1; ++i) {
		try {
			if (!StringUtils::isDigits(args[i])) {
				throw std::invalid_argument("Error code must be a number.");
			}
			long code_long = StringUtils::stringToLong(args[i]);
			if (code_long < 100 || code_long > 599) {
				throw std::out_of_range("Error code out of valid HTTP status code range (100-599).");
			}
			// This overrides/adds to inherited error pages for specific codes.
			locationConfig.errorPages[static_cast<int>(code_long)] = uri;
		} catch (const std::invalid_argument& e) {
			error("Error code for 'error_page' invalid: " + std::string(e.what()),
				  directive->line, directive->column);
		} catch (const std::out_of_range& e) {
			error("Error code for 'error_page' " + std::string(e.what()),
				  directive->line, directive->column);
		}
	}
}

/**
 * @brief Handles the 'client_max_body_size' directive for a ServerConfig.
 * @param directive The 'client_max_body_size' DirectiveNode.
 * @param serverConfig The ServerConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleClientMaxBodySizeDirective(const DirectiveNode* directive, ServerConfig& serverConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: exactly one argument is required.
	if (args.size() != 1) {
		error("Directive 'client_max_body_size' requires exactly one argument (size with optional units).",
			  directive->line, directive->column);
	}
	try {
		// Use the helper function to parse the size string (e.g., "10m") into bytes.
		serverConfig.clientMaxBodySize = parseSizeToBytes(args[0]);
	} catch (const std::invalid_argument& e) {
		error("Invalid client_max_body_size format: " + std::string(e.what()),
			  directive->line, directive->column);
	} catch (const std::out_of_range& e) {
		error("Client_max_body_size value " + std::string(e.what()),
			  directive->line, directive->column);
	}
}

/**
 * @brief Handles the 'client_max_body_size' directive for a LocationConfig.
 * @param directive The 'client_max_body_size' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleClientMaxBodySizeDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	if (args.size() != 1) {
		error("Directive 'client_max_body_size' requires exactly one argument (size with optional units).",
			  directive->line, directive->column);
	}
	try {
		locationConfig.clientMaxBodySize = parseSizeToBytes(args[0]); // Overrides inherited value.
	} catch (const std::invalid_argument& e) {
		error("Invalid client_max_body_size format: " + std::string(e.what()),
			  directive->line, directive->column);
	} catch (const std::out_of_range& e) {
		error("Client_max_body_size value " + std::string(e.what()),
			  directive->line, directive->column);
	}
}

// --- Location-Specific Handlers ---

/**
 * @brief Handles the 'allowed_methods' directive for a LocationConfig.
 * @param directive The 'allowed_methods' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleAllowedMethodsDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: at least one method must be provided.
	if (args.empty()) {
		error("Directive 'allowed_methods' requires at least one argument (HTTP method).",
			  directive->line, directive->column);
	}

	// Clear any previously allowed methods (this directive completely redefines the list).
	locationConfig.allowedMethods.clear();
	
	// Convert each argument string to HttpMethod enum and add to the list.
	for (size_t i = 0; i < args.size(); ++i) {
		try {
			locationConfig.allowedMethods.push_back(stringToHttpMethod(args[i]));
		} catch (const std::invalid_argument& e) {
			error("Invalid HTTP method '" + args[i] + "'. " + std::string(e.what()),
				  directive->line, directive->column);
		}
	}
}

/**
 * @brief Handles the 'upload_enabled' directive for a LocationConfig.
 * @param directive The 'upload_enabled' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleUploadEnabledDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: exactly one argument required.
	if (args.size() != 1) {
		error("Directive 'upload_enabled' requires exactly one argument ('on' or 'off').",
			  directive->line, directive->column);
	}
	// Validate argument value.
	if (args[0] == "on") {
		locationConfig.uploadEnabled = true;
	} else if (args[0] == "off") {
		locationConfig.uploadEnabled = false;
	} else {
		error("Argument for 'upload_enabled' must be 'on' or 'off', but got '" + args[0] + "'.",
			  directive->line, directive->column);
	}
}

/**
 * @brief Handles the 'upload_store' directive for a LocationConfig.
 * @param directive The 'upload_store' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleUploadStoreDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: exactly one argument required.
	if (args.size() != 1) {
		error("Directive 'upload_store' requires exactly one argument (directory path).",
			  directive->line, directive->column);
	}
	// Basic path validation: must not be empty.
	if (args[0].empty()) {
		error("Upload store path cannot be empty.", directive->line, directive->column);
	}
	locationConfig.uploadStore = args[0]; // Assign the upload store path.
}

/**
 * @brief Handles the 'cgi_extension' directive for a LocationConfig.
 * This directive defines file extensions that should be treated as CGI.
 * The actual executable path is set by 'cgi_path'.
 * @param directive The 'cgi_extension' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleCgiExtensionDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: at least one extension must be provided.
	if (args.empty()) {
		error("Directive 'cgi_extension' requires at least one argument (file extension).",
			  directive->line, directive->column);
	}

	// Add each specified extension to the cgiExecutables map.
	// At this stage, the executable path for these extensions might not be known yet.
	// They will be filled in when a 'cgi_path' directive is encountered.
	// If a cgi_extension is provided without a cgi_path, the mapped value will temporarily be empty.
	for (size_t i = 0; i < args.size(); ++i) {
		const std::string& ext = args[i];
		if (ext.empty() || ext[0] != '.') { // Basic validation: must start with a dot.
			error("CGI extension '" + ext + "' must start with a dot (e.g., '.php').",
				  directive->line, directive->column);
		}
		// If a cgi_path was already defined, use it. Otherwise, set an empty string as placeholder.
		// The default cgi_path for all extensions added by this directive will be filled by handleCgiPathDirective.
		locationConfig.cgiExecutables[ext] = ""; // Initialize with empty string, to be filled by cgi_path
	}
}

/**
 * @brief Handles the 'cgi_path' directive for a LocationConfig.
 * This directive defines the executable path for CGI scripts.
 * It updates the cgiExecutables map for all previously defined extensions.
 * @param directive The 'cgi_path' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleCgiPathDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: exactly one argument required.
	if (args.size() != 1) {
		error("Directive 'cgi_path' requires exactly one argument (path to CGI executable).",
			  directive->line, directive->column);
	}
	// Basic path validation: must not be empty.
	const std::string& cgi_path = args[0];
	if (cgi_path.empty()) {
		error("CGI path cannot be empty.", directive->line, directive->column);
	}

	// If no CGI extensions have been defined yet, it's an error.
	// A cgi_path without corresponding cgi_extension is usually meaningless in config.
	if (locationConfig.cgiExecutables.empty()) {
		error("Directive 'cgi_path' found without preceding 'cgi_extension' directives.",
			  directive->line, directive->column);
	}

	// Update all currently mapped CGI extensions with this path.
	// This assumes that 'cgi_path' applies to all 'cgi_extension' directives
	// encountered so far within this location block.
	std::map<std::string, std::string>::iterator it;
	for (it = locationConfig.cgiExecutables.begin(); it != locationConfig.cgiExecutables.end(); ++it) {
		it->second = cgi_path;
	}
}

/**
 * @brief Handles the 'return' directive for a LocationConfig.
 * @param directive The 'return' DirectiveNode.
 * @param locationConfig The LocationConfig object to update.
 * @throws ConfigLoadError if arguments are invalid.
 */
void ConfigLoader::handleReturnDirective(const DirectiveNode* directive, LocationConfig& locationConfig) {
	const std::vector<std::string>& args = directive->args;

	// Validate argument count: 1 (status_code) or 2 (status_code + URL/text).
	if (args.empty() || args.size() > 2) {
		error("Directive 'return' requires one or two arguments: a status code and optional URL/text.",
			  directive->line, directive->column);
	}

	// First argument is the status code.
	try {
		if (!StringUtils::isDigits(args[0])) {
			throw std::invalid_argument("Status code must be a number.");
		}
		long code_long = StringUtils::stringToLong(args[0]);
		// Validate standard HTTP status code range (e.g., typically 100-599).
		// For 'return', it's often redirection codes (3xx) but can be others too.
		if (code_long < 100 || code_long > 599) {
			throw std::out_of_range("Status code out of valid HTTP status code range (100-599).");
		}
		locationConfig.returnCode = static_cast<int>(code_long);
	} catch (const std::invalid_argument& e) {
		error("Status code for 'return' invalid: " + std::string(e.what()),
			  directive->line, directive->column);
	} catch (const std::out_of_range& e) {
		error("Status code for 'return' " + std::string(e.what()),
			  directive->line, directive->column);
	}

	// If a second argument is present, it's the URL or text.
	if (args.size() == 2) {
		locationConfig.returnUrlOrText = args[1];
		if (locationConfig.returnUrlOrText.empty()) {
			error("Return URL/text cannot be empty if provided.", directive->line, directive->column);
		}
	} else {
		locationConfig.returnUrlOrText = ""; // Ensure it's empty if not provided.
	}
}

// --- General Utility/Conversion Functions (Members of ConfigLoader) ---

/**
 * @brief Converts a string HTTP method (e.g., "GET") to its HttpMethod enum.
 * @param methodStr The string representation of the HTTP method.
 * @return The corresponding HttpMethod enum value.
 * @throws std::invalid_argument if the method string is not a recognized HTTP method.
 */
HttpMethod ConfigLoader::stringToHttpMethod(const std::string& methodStr) const {
	if (methodStr == "GET") return GET_METHOD;
	if (methodStr == "POST") return POST_METHOD;
	if (methodStr == "DELETE") return DELETE_METHOD;
	// Add other methods if your server supports them beyond the minimum requirement.
	// if (methodStr == "PUT") return PUT_METHOD;
	// if (methodStr == "HEAD") return HEAD_METHOD;
	// if (methodStr == "OPTIONS") return OPTIONS_METHOD;

	throw std::invalid_argument("Unknown HTTP method '" + methodStr + "'.");
	// return UNKNOWN_METHOD; // This line is not reached due to throw
}

/**
 * @brief Converts a string log level (e.g., "error") to its LogLevel enum.
 * @param levelStr The string representation of the log level.
 * @return The corresponding LogLevel enum value.
 * @throws std::invalid_argument if the level string is not a recognized log level.
 */
LogLevel ConfigLoader::stringToLogLevel(const std::string& levelStr) const {
	// Convert to lowercase for case-insensitive comparison, mirroring Nginx behavior.
	std::string lowerLevelStr = levelStr;
	for (size_t i = 0; i < lowerLevelStr.length(); ++i) {
		lowerLevelStr[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowerLevelStr[i])));
	}

	if (lowerLevelStr == "debug") return DEBUG_LOG;
	if (lowerLevelStr == "info") return INFO_LOG;
	if (lowerLevelStr == "warn") return WARN_LOG;
	if (lowerLevelStr == "error") return ERROR_LOG;
	if (lowerLevelStr == "crit") return CRIT_LOG;
	if (lowerLevelStr == "alert") return ALERT_LOG;
	if (lowerLevelStr == "emerg") return EMERG_LOG;

	throw std::invalid_argument("Unknown log level '" + levelStr + "'. Expected debug, info, warn, error, crit, alert, or emerg.");
	// return DEFAULT_LOG; // This line is not reached due to throw
}

/**
 * @brief Parses a size string (e.g., "10m", "512k", "2g") into bytes.
 * @param sizeStr The string representation of the size.
 * @return The size in bytes as a long.
 * @throws std::invalid_argument if the size string format is invalid.
 * @throws std::out_of_range if the value overflows long.
 */
long ConfigLoader::parseSizeToBytes(const std::string& sizeStr) const {
	if (sizeStr.empty()) {
		throw std::invalid_argument("Size string cannot be empty.");
	}

	size_t i = 0;
	while (i < sizeStr.length() && std::isdigit(static_cast<unsigned char>(sizeStr[i]))) {
		i++;
	}

	if (i == 0) { // No digits found at the beginning.
		throw std::invalid_argument("Size string must start with a number: '" + sizeStr + "'.");
	}

	std::string num_part = sizeStr.substr(0, i);
	std::string unit_part = sizeStr.substr(i);

	long value = 0;
	// stringToLong itself throws std::invalid_argument or std::out_of_range
	value = StringUtils::stringToLong(num_part);

	long multiplier = 1; // Default to bytes
	if (!unit_part.empty()) {
		if (unit_part.length() > 1) { // Only single-character units like 'k', 'm', 'g' are expected.
			throw std::invalid_argument("Invalid unit format in size string: '" + unit_part + "'. Expected 'k', 'm', or 'g'.");
		}
		char unit = static_cast<char>(std::tolower(static_cast<unsigned char>(unit_part[0])));
		if (unit == 'k') {
			multiplier = 1024;
		} else if (unit == 'm') {
			multiplier = 1024 * 1024;
		} else if (unit == 'g') {
			multiplier = 1024 * 1024 * 1024;
		} else {
			throw std::invalid_argument("Unknown unit '" + unit_part + "'. Expected 'k', 'm', or 'g'.");
		}
	}

	// Final check for potential overflow during multiplication by multiplier
	// It's safer to check before multiplication to prevent overflow if value * multiplier
	// would exceed max_long.
	if (multiplier > 1 && value > std::numeric_limits<long>::max() / multiplier) {
		throw std::out_of_range("Calculated size (positive overflow) exceeds maximum long value: " + sizeStr);
	}
	if (multiplier > 1 && value < std::numeric_limits<long>::min() / multiplier) {
		throw std::out_of_range("Calculated size (negative overflow) exceeds minimum long value: " + sizeStr);
	}

	return value * multiplier;
}

// --- ConfigLoadError Helper ---

/**
 * @brief Throws a ConfigLoadError with the given message, line, and column.
 * @param msg The error message.
 * @param line The line number where the error occurred.
 * @param col The column number where the error occurred.
 */
void ConfigLoader::error(const std::string& msg, int line, int col) const {
	std::ostringstream oss;
	oss << "Config Load Error at line " << line << ", col " << col << ": " << msg;
	throw ConfigLoadError(oss.str(), line, col);
}
