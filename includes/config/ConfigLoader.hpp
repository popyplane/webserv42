#ifndef CONFIG_LOADER_HPP
# define CONFIG_LOADER_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <limits>

#include "ASTnode.hpp"
#include "ServerStructures.hpp"
#include "../utils/StringUtils.hpp"

class ConfigLoader {
public:
	ConfigLoader();
	~ConfigLoader();

	std::vector<ServerConfig> loadConfig(const std::vector<ASTnode*>& astNodes);

private:
	// --- Core Parsing Functions ---
	ServerConfig    parseServerBlock(const BlockNode* serverBlockNode);

	// Existing: Parses a top-level location block (inherits from ServerConfig)
	LocationConfig  parseLocationBlock(const BlockNode* locationBlockNode, const ServerConfig& parentServerDefaults);

	// NEW OVERLOAD: Parses a nested location block (inherits from LocationConfig)
	LocationConfig  parseLocationBlock(const BlockNode* locationBlockNode, const LocationConfig& parentLocationDefaults);


	// --- Dispatcher Functions ---
	void            processDirective(const DirectiveNode* directive, ServerConfig& serverConfig);
	void            processDirective(const DirectiveNode* directive, LocationConfig& locationConfig);


	// --- Dedicated Helper Functions for Each Directive's Specific Logic ---
	// (As defined in previous comments, these remain the same)

	// Server-specific directives
	void            handleListenDirective(const DirectiveNode* directive, ServerConfig& serverConfig);
	void            handleServerNameDirective(const DirectiveNode* directive, ServerConfig& serverConfig);
	void            handleErrorLogDirective(const DirectiveNode* directive, ServerConfig& serverConfig);

	// Directives common to both Server and Location contexts (overloaded)
	void            handleRootDirective(const DirectiveNode* directive, ServerConfig& serverConfig);
	void            handleRootDirective(const DirectiveNode* directive, LocationConfig& locationConfig);
	void            handleIndexDirective(const DirectiveNode* directive, ServerConfig& serverConfig);
	void            handleIndexDirective(const DirectiveNode* directive, LocationConfig& locationConfig);
	void            handleAutoindexDirective(const DirectiveNode* directive, ServerConfig& serverConfig);
	void            handleAutoindexDirective(const DirectiveNode* directive, LocationConfig& locationConfig);
	void            handleErrorPageDirective(const DirectiveNode* directive, ServerConfig& serverConfig);
	void            handleErrorPageDirective(const DirectiveNode* directive, LocationConfig& locationConfig);
	void            handleClientMaxBodySizeDirective(const DirectiveNode* directive, ServerConfig& serverConfig);
	void            handleClientMaxBodySizeDirective(const DirectiveNode* directive, LocationConfig& locationConfig);

	// Location-specific directives
	void            handleAllowedMethodsDirective(const DirectiveNode* directive, LocationConfig& locationConfig);
	void            handleUploadEnabledDirective(const DirectiveNode* directive, LocationConfig& locationConfig);
	void            handleUploadStoreDirective(const DirectiveNode* directive, LocationConfig& locationConfig);
	void            handleCgiExtensionDirective(const DirectiveNode* directive, LocationConfig& locationConfig);
	void            handleCgiPathDirective(const DirectiveNode* directive, LocationConfig& locationConfig);
	void            handleReturnDirective(const DirectiveNode* directive, LocationConfig& locationConfig);


	// --- General Utility/Conversion Functions (Members of ConfigLoader) ---
	HttpMethod      stringToHttpMethod(const std::string& methodStr) const;
	LogLevel        stringToLogLevel(const std::string& levelStr) const;
	long            parseSizeToBytes(const std::string& sizeStr) const;

	// --- Error Handling Helper ---
	void            error(const std::string& msg, int line, int col) const;
};

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
