/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequestHandler.cpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/25 09:57:18 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 15:51:35 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/HttpRequestHandler.hpp" // Include its own header
#include "../../includes/utils/StringUtils.hpp" // For StringUtils::trim, StringUtils::startsWith, StringUtils::endsWith, etc.

#include <iostream>    // For debug output
#include <sstream>     // For string manipulation
#include <cstdio>      // For snprintf, used for formatting
#include <sys/stat.h>  // For stat(), S_ISREG, S_ISDIR, mkdir()
#include <unistd.h>    // For access(), unlink()
#include <fstream>     // For std::ifstream, std::ofstream
#include <dirent.h>    // For opendir, readdir, closedir
#include <sys/time.h>  // For gettimeofday() for unique filename
#include <limits>      // For std::numeric_limits<long>::max()
#include <errno.h>     // For errno and strerror
#include <string.h>    // For strerror

// Constructor
HttpRequestHandler::HttpRequestHandler() {}

// Destructor
HttpRequestHandler::~HttpRequestHandler() {}

// --- Private Utility Methods for File System & Config Access ---

// Checks if a path points to a regular file
bool HttpRequestHandler::_isRegularFile(const std::string& path) const {
    struct stat fileStat;
    return stat(path.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode);
}

// Checks if a path points to a directory
bool HttpRequestHandler::_isDirectory(const std::string& path) const {
    struct stat fileStat;
    return stat(path.c_str(), &fileStat) == 0 && S_ISDIR(fileStat.st_mode);
}

// Checks if a file exists
bool HttpRequestHandler::_fileExists(const std::string& path) const {
    struct stat fileStat;
    return stat(path.c_str(), &fileStat) == 0;
}

// Checks if a file/directory has read permissions
bool HttpRequestHandler::_canRead(const std::string& path) const {
    return access(path.c_str(), R_OK) == 0;
}

// Checks if a directory has write permissions
bool HttpRequestHandler::_canWrite(const std::string& path) const {
    return access(path.c_str(), W_OK) == 0;
}


// Gets the effective root directory for a request (location root takes precedence)
std::string HttpRequestHandler::_getEffectiveRoot(const ServerConfig* server, const LocationConfig* location) const {
    // If a location is matched and has a specific root, use it.
    if (location && !location->root.empty()) {
        return location->root;
    }
    // Otherwise, fall back to the server-level root.
    if (server && !server->root.empty()) {
        return server->root;
    }
    // If neither has a root, return empty (should be caught by config validation ideally)
    return "";
}

// Gets the effective upload store path (location upload_store takes precedence)
std::string HttpRequestHandler::_getEffectiveUploadStore(const ServerConfig* server, const LocationConfig* location) const {
    (void)server; // Suppress unused parameter warning/error, as uploadStore is location-specific.
    if (location && !location->uploadStore.empty()) {
        return location->uploadStore;
    }
    // Upload store is typically location-specific; no fallback to server for this.
    return "";
}

// Gets the effective client max body size (location client_max_body_size takes precedence)
long HttpRequestHandler::_getEffectiveClientMaxBodySize(const ServerConfig* server, const LocationConfig* location) const {
    if (location && location->clientMaxBodySize != 0) { // Assuming 0 means not set/default in config
        return location->clientMaxBodySize;
    }
    if (server && server->clientMaxBodySize != 0) {
        return server->clientMaxBodySize;
    }
    return std::numeric_limits<long>::max(); // Default to effectively unlimited if neither specify
}


// Gets the effective error pages map (location error_page takes precedence)
const std::map<int, std::string>& HttpRequestHandler::_getEffectiveErrorPages(const ServerConfig* server, const LocationConfig* location) const {
    // If a location has specific error pages, use those.
    if (location && !location->errorPages.empty()) {
        return location->errorPages;
    }
    // Otherwise, fall back to the server's error pages.
    if (server) {
        return server->errorPages;
    }
    // Fallback if no server is matched (should be rare/handled earlier)
    static const std::map<int, std::string> emptyMap;
    return emptyMap;
}

// Determines the MIME type based on file extension
std::string HttpRequestHandler::_getMimeType(const std::string& filePath) const {
    size_t dotPos = filePath.rfind('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream"; // Default for no extension
    }
    std::string ext = filePath.substr(dotPos);
    // Convert extension to lowercase for case-insensitive matching
    std::transform(ext.begin(), ext.end(), ext.begin(), static_cast<int(*)(int)>(std::tolower));

    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".txt") return "text/plain";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".xml") return "application/xml";
    // Add more types as needed
    return "application/octet-stream"; // Default fallback
}


// --- Core Response Generation Logic ---

// Generates an error response HTML or serves a custom error page.
HttpResponse HttpRequestHandler::_generateErrorResponse(int statusCode,
                                                         const ServerConfig* serverConfig,
                                                         const LocationConfig* locationConfig) {
    HttpResponse response;
    response.setStatus(statusCode);
    response.addHeader("Content-Type", "text/html");

    // Try to serve a custom error page if configured
    const std::map<int, std::string>& errorPages = _getEffectiveErrorPages(serverConfig, locationConfig);
    std::map<int, std::string>::const_iterator it = errorPages.find(statusCode);

    if (it != errorPages.end() && !it->second.empty()) {
        // Build the path for the custom error page.
        // For simplicity, we assume error page paths are always relative to the main server root.
        std::string effectiveRootForError = _getEffectiveRoot(serverConfig, NULL); // Use server's root for error pages
        std::string customErrorPagePath = effectiveRootForError;
        if (!customErrorPagePath.empty() && customErrorPagePath[customErrorPagePath.length() - 1] == '/') {
            customErrorPagePath = customErrorPagePath.substr(0, customErrorPagePath.length() - 1);
        }
        customErrorPagePath += it->second; // it->second typically starts with '/'

        std::cout << "DEBUG: Trying to serve custom error page: " << customErrorPagePath << "\n";
        if (_isRegularFile(customErrorPagePath) && _canRead(customErrorPagePath)) {
            std::ifstream file(customErrorPagePath.c_str(), std::ios::in | std::ios::binary);
            if (file.is_open()) {
                std::vector<char> fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                response.setBody(fileContent);
                response.addHeader("Content-Type", _getMimeType(customErrorPagePath));
                file.close();
                return response; // Successfully served custom error page
            }
        }
        std::cerr << "WARNING: Failed to serve custom error page " << customErrorPagePath << ", serving generic.\n";
    }

    // Fallback: Generate a generic HTML error page
    std::ostringstream oss;
    oss << "<html><head><title>Error " << statusCode << "</title></head><body>"
        << "<h1>" << statusCode << " " << response.getStatusMessage() << "</h1>"
        << "<p>The webserv server encountered an error.</p>"
        << "</body></html>";
    response.setBody(oss.str());
    return response;
}

// Resolves the full file system path from a URI.
std::string HttpRequestHandler::_resolvePath(const std::string& uriPath,
                                             const ServerConfig* serverConfig,
                                             const LocationConfig* locationConfig) const {
    std::string effectiveRoot = _getEffectiveRoot(serverConfig, locationConfig);
    if (effectiveRoot.empty()) {
        std::cerr << "ERROR: No effective root found for URI: " << uriPath << std::endl;
        return ""; // Indicate failure to resolve path
    }

    // Ensure effectiveRoot does NOT end with a slash for consistent concatenation,
    // UNLESS it's just "/"
    if (effectiveRoot.length() > 1 && StringUtils::endsWith(effectiveRoot, "/")) {
        effectiveRoot = effectiveRoot.substr(0, effectiveRoot.length() - 1);
    }

    std::string relativeSuffix = uriPath;

    if (locationConfig) {
        // If the URI path is prefixed by the location path
        if (StringUtils::startsWith(uriPath, locationConfig->path)) {
            // Case 1: Location path is a directory-like prefix (ends with '/')
            // Example: locationConfig->path = "/images/", uriPath = "/images/logo.jpg"
            // relativeSuffix becomes "logo.jpg"
            if (StringUtils::endsWith(locationConfig->path, "/")) {
                relativeSuffix = uriPath.substr(locationConfig->path.length());
                // Ensure the relative suffix has a leading slash for proper joining, if it's not empty
                if (!relativeSuffix.empty() && relativeSuffix[0] != '/') {
                    relativeSuffix = "/" + relativeSuffix;
                } else if (relativeSuffix.empty() && uriPath == locationConfig->path) {
                    // Special case: if uriPath is exactly /images/ and location path is /images/
                    // then relativeSuffix should be just "/" to signify the directory itself.
                    relativeSuffix = "/";
                }
            }
            // Case 2: Location path is an exact file or non-directory match (does NOT end with '/')
            // Example: locationConfig->path = "/protected_file.txt", uriPath = "/protected_file.txt"
            // Example: locationConfig->path = "/status", uriPath = "/status"
            // In these cases, the entire locationConfig->path is the "relative" part.
            else if (uriPath == locationConfig->path) {
                // The relative path is effectively the filename part of the location path.
                // We want to append this filename directly to the effective root.
                size_t lastSlash = locationConfig->path.rfind('/');
                if (lastSlash != std::string::npos) {
                    relativeSuffix = locationConfig->path.substr(lastSlash); // includes leading slash for filename
                } else {
                    relativeSuffix = "/" + locationConfig->path; // For cases like "file.txt" as location path
                }
            } else {
                // This scenario means uriPath starts with locationConfig->path, but it's not
                // a directory-like prefix, nor an exact match.
                // Example: location /file, uri /file_extended - this is ambiguous
                // For now, treat relativeSuffix as the rest of the URI
                relativeSuffix = uriPath.substr(locationConfig->path.length());
                if (!relativeSuffix.empty() && relativeSuffix[0] != '/') {
                    relativeSuffix = "/" + relativeSuffix;
                }
            }
        } else {
            // This case implies a logic error in dispatcher or config if locationConfig is non-NULL
            // but uriPath doesn't start with locationConfig->path. Fallback to full uriPath.
            // This should ideally not be hit if RequestDispatcher works correctly.
            std::cerr << "WARNING: URI path '" << uriPath << "' does not start with matched location path '" << locationConfig->path << "'. Using full URI path as suffix.\n";
            // Ensure leading slash if uriPath doesn't have one and is not empty.
            if (!relativeSuffix.empty() && relativeSuffix[0] != '/') {
                relativeSuffix = "/" + relativeSuffix;
            }
        }
    } else {
        // No locationConfig, use server-level root. Ensure leading slash if uriPath doesn't have one.
        if (!relativeSuffix.empty() && relativeSuffix[0] != '/') {
            relativeSuffix = "/" + relativeSuffix;
        } else if (relativeSuffix.empty()) {
            relativeSuffix = "/"; // If uriPath was empty for some reason, resolve to root directory
        }
    }
    
    // Final concatenation. If relativeSuffix is "/", it means the request was for a directory.
    // If effectiveRoot ends in '/' already, avoid double slash.
    if (relativeSuffix == "/" && effectiveRoot.empty()) { // Edge case: if root is also empty
        return "/";
    }
    if (relativeSuffix == "/" && effectiveRoot.length() > 0 && effectiveRoot[effectiveRoot.length() - 1] != '/') {
        return effectiveRoot + "/";
    }
    return effectiveRoot + relativeSuffix;
}


// Handles a GET request.
HttpResponse HttpRequestHandler::_handleGet(const HttpRequest& request,
                                            const ServerConfig* serverConfig,
                                            const LocationConfig* locationConfig) {
    if (!serverConfig) {
        return _generateErrorResponse(500, NULL, NULL); // Should not happen after dispatcher
    }

    std::string fullPath = _resolvePath(request.path, serverConfig, locationConfig);
    std::cout << "DEBUG: Attempting to serve GET for URI: " << request.uri << " from resolved path: " << fullPath << "\n";

    if (fullPath.empty()) {
        return _generateErrorResponse(500, serverConfig, locationConfig); // Path resolution failed
    }

    // --- Case 1: Path is a Directory ---
    if (_isDirectory(fullPath)) {
        if (!_canRead(fullPath)) {
            std::cerr << "ERROR: Directory " << fullPath << " not readable.\n";
            return _generateErrorResponse(403, serverConfig, locationConfig); // Forbidden
        }

        // Try to serve index files if configured
        std::vector<std::string> indexFiles;
        // Use location-specific index files if available, else server-level
        if (locationConfig && !locationConfig->indexFiles.empty()) {
            indexFiles = locationConfig->indexFiles;
        } else if (serverConfig && !serverConfig->indexFiles.empty()) {
            indexFiles = serverConfig->indexFiles;
        }

        for (size_t i = 0; i < indexFiles.size(); ++i) {
            std::string indexPath = fullPath;
            if (indexPath[indexPath.length() - 1] != '/') {
                indexPath += "/";
            }
            indexPath += indexFiles[i];
            
            std::cout << "DEBUG: Trying index file: " << indexPath << "\n";
            if (_isRegularFile(indexPath) && _canRead(indexPath)) {
                std::ifstream file(indexPath.c_str(), std::ios::in | std::ios::binary);
                if (file.is_open()) {
                    HttpResponse response;
                    response.setStatus(200);
                    std::vector<char> fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    response.setBody(fileContent);
                    response.addHeader("Content-Type", _getMimeType(indexPath));
                    file.close();
                    return response;
                }
            }
        }
        
        // If no index file found, check autoindex
        bool autoindexEnabled = false;
        if (locationConfig && locationConfig->autoindex) { // Location autoindex takes precedence
            autoindexEnabled = true;
        } else if (serverConfig && serverConfig->autoindex) { // Fallback to server autoindex
            autoindexEnabled = true;
        }

        if (autoindexEnabled) {
            std::cout << "DEBUG: Autoindex enabled. Generating directory listing for: " << fullPath << "\n";
            HttpResponse response;
            response.setStatus(200);
            response.addHeader("Content-Type", "text/html");
            response.setBody(_generateAutoindexPage(fullPath, request.path));
            return response;
        } else {
            std::cerr << "ERROR: Autoindex off and no index file for directory: " << fullPath << "\n";
            return _generateErrorResponse(403, serverConfig, locationConfig); // Forbidden (Directory listing not allowed)
        }
    }
    // --- Case 2: Path is a Regular File ---
    else if (_isRegularFile(fullPath)) {
        if (!_canRead(fullPath)) {
            std::cerr << "ERROR: File " << fullPath << " not readable.\n";
            return _generateErrorResponse(403, serverConfig, locationConfig); // Forbidden
        }
        
        std::ifstream file(fullPath.c_str(), std::ios::in | std::ios::binary);
        if (file.is_open()) {
            HttpResponse response;
            response.setStatus(200);
            std::vector<char> fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            response.setBody(fileContent);
            response.addHeader("Content-Type", _getMimeType(fullPath));
            file.close();
            return response;
        } else {
            // Should be caught by _canRead or _isRegularFile, but defensive.
            std::cerr << "ERROR: Failed to open file: " << fullPath << "\n";
            return _generateErrorResponse(500, serverConfig, locationConfig); // Internal Server Error
        }
    }
    // --- Case 3: Path does not exist or is not a regular file/directory ---
    else {
        std::cerr << "ERROR: Path not found or not a regular file/directory: " << fullPath << "\n";
        return _generateErrorResponse(404, serverConfig, locationConfig); // Not Found
    }
}

// Handles a POST request (specifically for file uploads).
HttpResponse HttpRequestHandler::_handlePost(const HttpRequest& request,
                                             const ServerConfig* serverConfig,
                                             const LocationConfig* locationConfig) {
    // 1. Get effective upload configuration
    std::string uploadStore = _getEffectiveUploadStore(serverConfig, locationConfig);
    long maxBodySize = _getEffectiveClientMaxBodySize(serverConfig, locationConfig);

    // Check if uploads are even enabled for this location.
    // Assuming locationConfig->uploadEnabled is true for /upload.
    // If you add an 'upload_enabled' flag to LocationConfig, you'd check it here.
    // For now, we proceed assuming the dispatcher already checked 'allowedMethods'

    // 2. Validate upload directory and permissions
    if (uploadStore.empty()) {
        std::cerr << "ERROR: POST request to " << request.uri << " failed: No upload_store configured for matched location.\n";
        return _generateErrorResponse(500, serverConfig, locationConfig); // Server misconfiguration
    }
    
    // Check if upload directory exists, if not, try to create it
    if (!_fileExists(uploadStore)) {
        // Permissions 0755: rwxr-xr-x (owner can read/write/execute, group/others can read/execute)
        // This is a common default for directories that need to be web-writable.
        if (mkdir(uploadStore.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
            std::cerr << "ERROR: POST request to " << request.uri << " failed: Upload store path is not a directory or cannot be created: " << uploadStore << ". Errno: " << strerror(errno) << "\n";
            return _generateErrorResponse(500, serverConfig, locationConfig); // Internal Server Error
        }
        std::cout << "INFO: Created upload directory: " << uploadStore << "\n";
    } else if (!_isDirectory(uploadStore)) {
        std::cerr << "ERROR: POST request to " << request.uri << " failed: Upload store path exists but is not a directory: " << uploadStore << "\n";
        return _generateErrorResponse(500, serverConfig, locationConfig); // Internal Server Error
    }

    if (!_canWrite(uploadStore)) {
        std::cerr << "ERROR: POST request to " << request.uri << " failed: Upload store directory not writable: " << uploadStore << "\n";
        return _generateErrorResponse(403, serverConfig, locationConfig); // Forbidden
    }

    // 3. Check Content-Length against client_max_body_size
    std::string contentLengthStr = request.getHeader("content-length");
    long contentLength = 0;
    if (!contentLengthStr.empty()) {
        try {
            contentLength = StringUtils::stringToLong(contentLengthStr);
        } catch (const std::exception& e) {
            std::cerr << "ERROR: POST request to " << request.uri << " failed: Invalid Content-Length header: " << contentLengthStr << ". " << e.what() << "\n";
            return _generateErrorResponse(400, serverConfig, locationConfig); // Bad Request
        }
    } else {
        // If no Content-Length, but there's a body, it's a protocol error (unless chunked, which we don't handle yet)
        if (!request.body.empty()) {
            std::cerr << "ERROR: POST request to " << request.uri << " failed: Missing Content-Length header with non-empty body.\n";
            return _generateErrorResponse(411, serverConfig, locationConfig); // Length Required
        }
    }
    
    // If request body size exceeds maxBodySize, return 413
    if (contentLength > maxBodySize) {
        std::cerr << "ERROR: POST request to " << request.uri << " failed: Payload too large (" << contentLength << " bytes > " << maxBodySize << " bytes).\n";
        return _generateErrorResponse(413, serverConfig, locationConfig); // Payload Too Large
    }

    // 4. Generate a unique filename
    std::string originalFilename = "uploaded_file"; // Default if no filename provided
    std::string contentDisposition = request.getHeader("content-disposition");
    // This is a basic attempt to parse filename from Content-Disposition header (e.g., for multipart/form-data)
    size_t filenamePos = contentDisposition.find("filename=");
    if (filenamePos != std::string::npos) {
        size_t start = contentDisposition.find('"', filenamePos);
        size_t end = std::string::npos;
        if (start != std::string::npos) {
            start++; // Move past the opening quote
            end = contentDisposition.find('"', start);
        }
        if (start != std::string::npos && end != std::string::npos) {
            originalFilename = contentDisposition.substr(start, end - start);
        }
    }
    
    // Basic sanitization of filename to prevent path traversal or other issues
    // Remove any path separators or ".."
    StringUtils::trim(originalFilename);
    size_t lastSlash = originalFilename.rfind('/');
    if (lastSlash != std::string::npos) {
        originalFilename = originalFilename.substr(lastSlash + 1);
    }
    lastSlash = originalFilename.rfind('\\'); // For Windows paths
    if (lastSlash != std::string::npos) {
        originalFilename = originalFilename.substr(lastSlash + 1);
    }
    // Remove ".." for robustness (though already done by substr after last slash for most cases)
    if (originalFilename.find("..") != std::string::npos) {
        originalFilename = StringUtils::split(originalFilename, '.')[0]; // Just take first part
        if (originalFilename.empty()) originalFilename = "sanitized_file";
    }
    if (originalFilename.empty()) {
        originalFilename = "unnamed_file";
    }


    struct timeval tv;
    gettimeofday(&tv, NULL);
    std::ostringstream uniqueNameStream;
    uniqueNameStream << tv.tv_sec << "_" << tv.tv_usec << "_" << originalFilename;
    std::string uniqueFilename = uniqueNameStream.str();
    
    std::string fullUploadPath = uploadStore;
    if (fullUploadPath[fullUploadPath.length() - 1] != '/') {
        fullUploadPath += "/";
    }
    fullUploadPath += uniqueFilename;

    // 5. Write the request body to the file
    // Using std::ios::trunc to create/overwrite the file.
    std::ofstream outputFile(fullUploadPath.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!outputFile.is_open()) {
        std::cerr << "ERROR: POST request to " << request.uri << " failed: Could not open file for writing: " << fullUploadPath << ". Errno: " << strerror(errno) << "\n";
        return _generateErrorResponse(500, serverConfig, locationConfig); // Internal Server Error
    }

    // Write the raw body content. request.body is a std::vector<char>
    if (!request.body.empty()) {
        outputFile.write(request.body.data(), request.body.size());
    }
    outputFile.close();

    if (outputFile.fail()) { // Check for errors after closing
        std::cerr << "ERROR: POST request to " << request.uri << " failed: Error writing to file: " << fullUploadPath << ". Errno: " << strerror(errno) << "\n";
        return _generateErrorResponse(500, serverConfig, locationConfig); // Internal Server Error
    }

    std::cout << "INFO: Successfully uploaded file to: " << fullUploadPath << "\n";

    // 6. Send 201 Created response
    HttpResponse response;
    response.setStatus(201); // 201 Created
    
    // Construct the Location header: /upload/original_filename (without timestamp for TC12)
    std::string locationHeaderUri = request.uri;
    if (!StringUtils::endsWith(locationHeaderUri, "/")) {
        locationHeaderUri += "/"; // Ensure trailing slash for directory upload URI
    }
    // FIX for TC12: Use originalFilename for Location header, not uniqueFilename
    locationHeaderUri += originalFilename;

    response.addHeader("Location", locationHeaderUri);
    response.addHeader("Content-Type", "text/html");
    
    std::ostringstream responseBody;
    responseBody << "<html><body><h1>201 Created</h1><p>File uploaded successfully: <a href=\""
                 << locationHeaderUri << "\">" << originalFilename << "</a></p></body></html>"; // Use original filename in body text
    response.setBody(responseBody.str());
    return response;
}

// Handles a DELETE request.
HttpResponse HttpRequestHandler::_handleDelete(const HttpRequest& request,
                                               const ServerConfig* serverConfig,
                                               const LocationConfig* locationConfig) {
    std::string fullPath;

    // Determine the full path for deletion based on whether it's within an upload_store location
    // This is crucial: if a location block for /uploads exists and has an upload_store,
    // DELETE requests to /uploads/filename.txt should delete from upload_store/filename.txt
    if (locationConfig && !locationConfig->uploadStore.empty() && StringUtils::startsWith(request.path, locationConfig->path)) {
        // Example: locationConfig->path = "/uploads", request.path = "/uploads/my_file.txt"
        // relativePath will be "my_file.txt"
        std::string relativePath = request.path.substr(locationConfig->path.length());
        
        // Ensure the uploadStore path has a trailing slash for consistent concatenation
        std::string baseUploadPath = locationConfig->uploadStore;
        if (!baseUploadPath.empty() && baseUploadPath[baseUploadPath.length() - 1] != '/') {
            baseUploadPath += "/";
        }

        // Handle leading slash in relativePath correctly to avoid double slashes.
        // If relativePath is "/my_file.txt", remove the leading '/'.
        // If relativePath is "", it means request.path was exactly locationConfig->path.
        if (!relativePath.empty() && relativePath[0] == '/') {
            relativePath = relativePath.substr(1);
        }
        fullPath = baseUploadPath + relativePath;

    } else {
        // Otherwise, use the standard root-based path resolution for deletion.
        // This handles cases where DELETE might be allowed on other static files.
        fullPath = _resolvePath(request.path, serverConfig, locationConfig);
    }

    std::cout << "DEBUG: Attempting to DELETE URI: " << request.uri << " from resolved path: " << fullPath << "\n";

    if (fullPath.empty()) {
        std::cerr << "ERROR: DELETE request for " << request.uri << " failed: Path resolution returned empty.\n";
        return _generateErrorResponse(500, serverConfig, locationConfig); // Path resolution failed
    }

    // 2. Check if the file exists
    if (!_fileExists(fullPath)) {
        std::cerr << "ERROR: DELETE request for " << request.uri << " failed: File not found at " << fullPath << "\n";
        return _generateErrorResponse(404, serverConfig, locationConfig); // Not Found
    }

    // 3. Ensure it's a regular file and not a directory (we only delete files)
    if (!_isRegularFile(fullPath)) {
        std::cerr << "ERROR: DELETE request for " << request.uri << " failed: Path is not a regular file (e.g., it's a directory): " << fullPath << "\n";
        // It's generally a 403 Forbidden to try to delete a directory with DELETE, or 409 Conflict.
        // For now, 403 is acceptable per common server behavior for this type of misrequest.
        return _generateErrorResponse(403, serverConfig, locationConfig);
    }

    // 4. Check write permissions on the file itself AND the parent directory
    // `std::remove` (which wraps `unlink`) requires write permissions on the *parent directory*
    // to remove an entry, and execute permissions to traverse.
    // It also needs write permission on the file itself if the file is immutable (e.g., ACLs).
    // The check for `access(fullPath.c_str(), W_OK)` is a good initial check for file writability,
    // but the directory permission is more critical for `remove`.

    // Check write permissions on the parent directory
    size_t lastSlashPos = fullPath.rfind('/');
    std::string parentDir;
    if (lastSlashPos != std::string::npos) {
        parentDir = fullPath.substr(0, lastSlashPos);
    } else { // File is in the root directory (e.g. "/file.txt")
        parentDir = "/";
    }

    if (!_canWrite(parentDir)) {
        std::cerr << "ERROR: DELETE request for " << request.uri << " failed: No write permissions on parent directory " << parentDir << " for file " << fullPath << ". Errno: " << strerror(errno) << "\n";
        return _generateErrorResponse(403, serverConfig, locationConfig); // Forbidden
    }
    
    // As a defensive measure, explicitly check write permission on the file itself.
    // If the file is not writable, even if the parent directory is, we should return 403.
    if (!_canWrite(fullPath)) {
        std::cerr << "ERROR: DELETE request for " << request.uri << " failed: No write permissions on file " << fullPath << ". Errno: " << strerror(errno) << "\n";
        return _generateErrorResponse(403, serverConfig, locationConfig); // Forbidden
    }


    // 5. Attempt to delete the file
    if (std::remove(fullPath.c_str()) != 0) {
        std::cerr << "ERROR: DELETE request for " << request.uri << " failed: Error deleting file " << fullPath << ". Errno: " << strerror(errno) << "\n";
        // Provide more specific error codes if `errno` indicates known issues
        if (errno == EACCES) { // Permission denied (e.g., parent dir not writable, or file is immutable)
            return _generateErrorResponse(403, serverConfig, locationConfig);
        } else if (errno == ENOENT) { // No such file or directory (race condition, file deleted between checks)
            return _generateErrorResponse(404, serverConfig, locationConfig);
        } else if (errno == EPERM) { // Operation not permitted (e.g., directory trying to be deleted as file)
            return _generateErrorResponse(403, serverConfig, locationConfig);
        }
        return _generateErrorResponse(500, serverConfig, locationConfig); // Generic Internal Server Error
    }

    std::cout << "INFO: Successfully deleted file: " << fullPath << "\n";

    // 6. Send 204 No Content response
    HttpResponse response;
    response.setStatus(204); // 204 No Content (commonly used for successful DELETE with no response body)
    return response;
}

// Generates an HTML page listing directory contents.
std::string HttpRequestHandler::_generateAutoindexPage(const std::string& directoryPath, const std::string& uriPath) const {
    std::ostringstream oss;
    oss << "<html><head><title>Index of " << uriPath << "</title>"
        << "<style>"
        << "body { font-family: sans-serif; background-color: #f0f0f0; margin: 2em; }"
        << "h1 { color: #333; }"
        << "ul { list-style-type: none; padding: 0; }"
        << "li { margin-bottom: 0.5em; }"
        << "a { text-decoration: none; color: #007bff; }"
        << "a:hover { text-decoration: underline; }"
        << ".parent-dir { font-weight: bold; color: #dc3545; }"
        << "</style>"
        << "</head><body>"
        << "<h1>Index of " << uriPath << "</h1><ul>";

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(directoryPath.c_str())) != NULL) {
        // Add a link to the parent directory, if not at the root URI
        if (uriPath != "/") {
            size_t lastSlash = uriPath.rfind('/', uriPath.length() - 2); // Find second to last slash
            std::string parentUri = uriPath.substr(0, lastSlash + 1);
            oss << "<li><a href=\"" << parentUri << "\" class=\"parent-dir\">.. (Parent Directory)</a></li>";
        }

        // Loop through all entries in the directory
        while ((ent = readdir(dir)) != NULL) {
            std::string name = ent->d_name;
            // Skip current directory (.) and parent directory (..)
            if (name == "." || name == "..") {
                continue;
            }

            std::string fullEntryPath = directoryPath;
            if (fullEntryPath[fullEntryPath.length() - 1] != '/') {
                fullEntryPath += "/";
            }
            fullEntryPath += name;

            std::string entryUri = uriPath;
            if (entryUri[entryUri.length() - 1] != '/') {
                entryUri += "/";
            }
            entryUri += name;

            oss << "<li><a href=\"" << entryUri;
            if (_isDirectory(fullEntryPath)) {
                oss << "/"; // Add trailing slash for directories in links
            }
            oss << "\">" << name;
            if (_isDirectory(fullEntryPath)) {
                oss << "/";
            }
            oss << "</a></li>";
        }
        closedir(dir);
    } else {
        // Should ideally be caught by _canRead / _isDirectory, but defensive
        std::cerr << "ERROR: Failed to open directory for autoindex: " << directoryPath << "\n";
        oss << "<li>Error: Could not open directory.</li>";
    }

    oss << "</ul></body></html>";
    return oss.str();
}

// --- Main Request Handling Method ---
HttpResponse HttpRequestHandler::handleRequest(const HttpRequest& request, const MatchedConfig& matchedConfig) {
    const ServerConfig* serverConfig = matchedConfig.server_config;
    const LocationConfig* locationConfig = matchedConfig.location_config;

    // --- Initial validation (should mostly be handled by dispatcher, but defensive) ---
    if (!serverConfig) {
        std::cerr << "ERROR: handleRequest called with NULL serverConfig. Returning 500.\n";
        return _generateErrorResponse(500, NULL, NULL);
    }
    // If no specific location matches, the locationConfig will be NULL.
    // The handler must be prepared to use server-level defaults.

    // --- 1. Handle Redirections (if configured) ---
    if (locationConfig && locationConfig->returnCode != 0) {
        HttpResponse response;
        response.setStatus(locationConfig->returnCode);
        response.addHeader("Location", locationConfig->returnUrlOrText);
        response.setBody("Redirecting to " + locationConfig->returnUrlOrText); // Simple body
        std::cout << "DEBUG: Redirecting " << request.uri << " to " << locationConfig->returnUrlOrText << " with status " << locationConfig->returnCode << "\n";
        return response;
    }

    // --- 2. Check Allowed Methods ---
    // Get effective allowed methods. If location has them, use; else, assume all methods (GET/POST/DELETE) for default
    std::vector<HttpMethod> allowedMethods;
    if (locationConfig && !locationConfig->allowedMethods.empty()) {
        allowedMethods = locationConfig->allowedMethods;
    } else {
        // Default: If no allowed_methods specified in location, assume GET, POST, DELETE are generally allowed
        // (This is a simplified default; often server-level defaults are more explicit).
        // For project requirements, you might need to make this default explicit:
        allowedMethods.push_back(HTTP_GET);
        allowedMethods.push_back(HTTP_POST);
        allowedMethods.push_back(HTTP_DELETE);
    }

    bool methodAllowed = false;
    HttpMethod reqMethodEnum;
    // Convert request.method string to enum for comparison
    if (request.method == "GET") reqMethodEnum = HTTP_GET;
    else if (request.method == "POST") reqMethodEnum = HTTP_POST;
    else if (request.method == "DELETE") reqMethodEnum = HTTP_DELETE;
    else reqMethodEnum = HTTP_UNKNOWN; // For unsupported methods by your server

    for (size_t i = 0; i < allowedMethods.size(); ++i) {
        if (allowedMethods[i] == reqMethodEnum) {
            methodAllowed = true;
            break;
        }
    }

    if (!methodAllowed) {
        std::cerr << "ERROR: Method " << request.method << " not allowed for path " << request.path << "\n";
        HttpResponse response = _generateErrorResponse(405, serverConfig, locationConfig); // Method Not Allowed
        // Build the Allow header string for 405 response
        std::string allowHeaderValue;
        for (size_t i = 0; i < allowedMethods.size(); ++i) {
            if (i > 0) allowHeaderValue += ", ";
            // Assuming you have a way to convert HttpMethod enum to string (e.g., httpMethodToString)
            if (allowedMethods[i] == HTTP_GET) allowHeaderValue += "GET";
            else if (allowedMethods[i] == HTTP_POST) allowHeaderValue += "POST";
            else if (allowedMethods[i] == HTTP_DELETE) allowHeaderValue += "DELETE";
        }
        response.addHeader("Allow", allowHeaderValue);
        return response;
    }


    // --- 3. Handle Request Method ---
    if (request.method == "GET") {
        return _handleGet(request, serverConfig, locationConfig);
    } else if (request.method == "POST") {
        return _handlePost(request, serverConfig, locationConfig);
    } else if (request.method == "DELETE") {
        return _handleDelete(request, serverConfig, locationConfig);
    } else {
        // Unknown or unsupported method by server
        std::cerr << "ERROR: Unsupported method: " << request.method << "\n";
        return _generateErrorResponse(501, serverConfig, locationConfig); // Not Implemented
    }
}
