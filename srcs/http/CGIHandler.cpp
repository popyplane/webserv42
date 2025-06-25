/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/25 17:47:11 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 23:25:10 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/CGIHandler.hpp"
#include "../../includes/http/HttpRequest.hpp" // For HttpRequest definition
#include "../../includes/config/ServerStructures.hpp" // For ServerConfig and LocationConfig definitions
#include "../../includes/utils/StringUtils.hpp" // For StringUtils utilities

#include <iostream>
#include <sys/socket.h> // For socketpair (though pipes are used here, good to have)
#include <sys/wait.h>   // Corrected: changed from <sys/wait.S> to <sys/wait.h>
#include <sys/stat.h>   // For stat (chdir uses this implicitly sometimes)
#include <cstdio>       // For remove
#include <signal.h>     // For kill()
#include <fcntl.h>      // For fcntl, O_NONBLOCK (explicitly added this, good practice)
#include <cstring>      // For strerror (explicitly added this, good practice)
#include <unistd.h>     // For close, dup2, execve, fork, usleep (explicitly added this, good practice)


// --- Constructor ---
CGIHandler::CGIHandler(const HttpRequest& request,
                       const ServerConfig* serverConfig,
                       const LocationConfig* locationConfig)
    : _request(request),
      _serverConfig(serverConfig),
      _locationConfig(locationConfig),
      _cgi_pid(-1),
      _request_body_sent_bytes(0),
      _request_body_ptr(&request.body), // Point to the request's body
      _state(CGIState::NOT_STARTED),
      _cgi_headers_parsed(false),
      _cgi_exit_status(-1)
{
    // Initialize pipe FDs to -1 to indicate they are not open
    _fd_stdin[0] = -1;
    _fd_stdin[1] = -1;
    _fd_stdout[0] = -1;
    _fd_stdout[1] = -1;

    // Determine CGI script path and executable path from config
    if (_locationConfig && !_locationConfig->root.empty() && !_locationConfig->cgiExecutables.empty()) {
        std::string locationRoot = _locationConfig->root;
        // Ensure locationRoot does NOT end with a slash for proper concatenation,
        // unless it's just "/" itself.
        if (locationRoot.length() > 1 && locationRoot[locationRoot.length() - 1] == '/') {
            locationRoot = locationRoot.substr(0, locationRoot.length() - 1);
        }

        // Extract the file extension from the request path
        size_t dot_pos = _request.path.rfind('.');
        if (dot_pos == std::string::npos) {
            std::cerr << "ERROR: CGIHandler: No file extension found in URI for CGI: " << _request.path << std::endl;
            _state = CGIState::CGI_PROCESS_ERROR;
            return;
        }
        std::string file_extension = _request.path.substr(dot_pos);
        
        // Find the CGI executable mapped to this extension
        std::map<std::string, std::string>::const_iterator cgi_it = _locationConfig->cgiExecutables.find(file_extension);
        if (cgi_it == _locationConfig->cgiExecutables.end()) {
            std::cerr << "ERROR: CGIHandler: No CGI executable configured for extension: " << file_extension << std::endl;
            _state = CGIState::CGI_PROCESS_ERROR;
            return;
        }
        _cgi_executable_path = cgi_it->second;

        // --- Corrected Logic for _cgi_script_path (retains previous good fix) ---
        // Combines location's root with the full URI path portion that maps to the script.
        std::string requestPathNormalized = _request.path;
        if (requestPathNormalized.empty() || requestPathNormalized[0] != '/') {
             requestPathNormalized = "/" + requestPathNormalized;
        }
        _cgi_script_path = locationRoot + requestPathNormalized;


        std::cout << "DEBUG: CGI script path resolved to: " << _cgi_script_path << std::endl;
        std::cout << "DEBUG: CGI executable path: " << _cgi_executable_path << std::endl;

    } else {
        std::cerr << "ERROR: CGIHandler: Incomplete location config for CGI setup." << std::endl;
        _state = CGIState::CGI_PROCESS_ERROR; // Mark as error early
    }

    // Set body pointer to NULL if request has no body (e.g., GET)
    if (_request.body.empty()) {
        _request_body_ptr = NULL;
    }
}

// --- Destructor ---
CGIHandler::~CGIHandler() {
    _closePipes(); // Ensure pipes are closed

    // If CGI process was forked and not yet waited for, wait for it
    if (_cgi_pid != -1) {
        int status;
        pid_t result = waitpid(_cgi_pid, &status, WNOHANG); // Check non-blocking first
        if (result == 0) { // Child still running
            std::cerr << "WARNING: CGI child process " << _cgi_pid << " still running in destructor, sending SIGTERM." << std::endl;
            kill(_cgi_pid, SIGTERM);
            waitpid(_cgi_pid, &status, 0); // Blocking wait to clean up zombie
        }
        // If result is _cgi_pid or -1, it already exited or waitpid errored, nothing more to do
    }
}

// Disallow copy constructor and assignment operator
CGIHandler::CGIHandler(const CGIHandler& other)
    : _request(other._request), // Reference copy
      _serverConfig(other._serverConfig),
      _locationConfig(other._locationConfig),
      _cgi_pid(-1), // Reset PID, pipes for new instance
      _request_body_sent_bytes(0),
      _request_body_ptr(other._request_body_ptr),
      _state(CGIState::NOT_STARTED),
      _cgi_headers_parsed(false),
      _cgi_exit_status(-1),
      _cgi_script_path(other._cgi_script_path),
      _cgi_executable_path(other._cgi_executable_path)
{
    _fd_stdin[0] = -1; _fd_stdin[1] = -1;
    _fd_stdout[0] = -1; _fd_stdout[1] = -1;
    // Note: This copy constructor is mostly for completeness as CGIHandler
    // objects should typically not be copied due to their resource ownership (PIDs, FDs).
}

CGIHandler& CGIHandler::operator=(const CGIHandler& other) {
    if (this != &other) {
        // Clean up current instance's resources
        _closePipes();
        if (_cgi_pid != -1) {
            kill(_cgi_pid, SIGTERM);
            waitpid(_cgi_pid, NULL, 0);
        }

        // Copy members
        // _request is a reference and cannot be reassigned.
        // It implies CGIHandler must manage the request lifetime carefully.
        // For project, assume request lifetime is guaranteed by owning context.
        _serverConfig = other._serverConfig;
        _locationConfig = other._locationConfig;
        _request_body_ptr = other._request_body_ptr;
        _cgi_script_path = other._cgi_script_path;
        _cgi_executable_path = other._cgi_executable_path;

        // Reset state for new instance
        _cgi_pid = -1;
        _fd_stdin[0] = -1; _fd_stdin[1] = -1;
        _fd_stdout[0] = -1; _fd_stdout[1] = -1;
        _request_body_sent_bytes = 0;
        _cgi_response_buffer.clear();
        _state = CGIState::NOT_STARTED;
        _cgi_headers_parsed = false;
        _cgi_exit_status = -1;
    }
    return *this;
}

// --- Private Helper: Set File Descriptor to Non-Blocking ---
bool CGIHandler::_setNonBlocking(int fd) {
    if (fd < 0) return false;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "ERROR: fcntl F_GETFL failed for FD " << fd << ": " << strerror(errno) << std::endl;
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "ERROR: fcntl F_SETFL O_NONBLOCK failed for FD " << fd << ": " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

// --- Private Helper: Create CGI Environment Variables ---
char** CGIHandler::_createCGIEnvironment() const {
    std::vector<std::string> env_vars_vec;

    // Mandatory CGI variables
    env_vars_vec.push_back("REQUEST_METHOD=" + _request.method);
    env_vars_vec.push_back("SERVER_PROTOCOL=" + _request.protocolVersion);

    // Add REDIRECT_STATUS to satisfy php-cgi's security check
    env_vars_vec.push_back("REDIRECT_STATUS=200"); // Common value used by Apache for internal redirects

    // SERVER_NAME and SERVER_PORT from matched server config
    if (_serverConfig) {
        if (!_serverConfig->serverNames.empty()) {
            env_vars_vec.push_back("SERVER_NAME=" + _serverConfig->serverNames[0]); // Use first server name
        } else {
            env_vars_vec.push_back("SERVER_NAME=localhost"); // Default if no server_name
        }
        env_vars_vec.push_back("SERVER_PORT=" + StringUtils::longToString(_serverConfig->port));
    } else {
        env_vars_vec.push_back("SERVER_NAME=unknown"); // Fallback
        env_vars_vec.push_back("SERVER_PORT=80"); // Fallback
    }

    // SCRIPT_FILENAME: Full file system path to the script.
    env_vars_vec.push_back("SCRIPT_FILENAME=" + _cgi_script_path); 

    // SCRIPT_NAME: The URI path to the script itself.
    env_vars_vec.push_back("SCRIPT_NAME=" + _request.path); 

    // PATH_INFO: Additional path information from the URI beyond the script name.
    // For test.php, PATH_INFO would typically be empty if request is /php/test.php
    // If request was /php/test.php/extra/path, then PATH_INFO = /extra/path
    // For our tests (/php/test.php), it is correctly empty.
    env_vars_vec.push_back("PATH_INFO="); 

    // REQUEST_URI: The full original request URI (including query string).
    env_vars_vec.push_back("REQUEST_URI=" + _request.uri); 

    // QUERY_STRING for GET requests
    size_t query_pos = _request.uri.find('?');
    if (query_pos != std::string::npos) {
        env_vars_vec.push_back("QUERY_STRING=" + _request.uri.substr(query_pos + 1));
    } else {
        env_vars_vec.push_back("QUERY_STRING=");
    }

    // CONTENT_TYPE and CONTENT_LENGTH for POST requests
    if (_request.method == "POST") {
        std::map<std::string, std::string>::const_iterator it_type = _request.headers.find("content-type");
        if (it_type != _request.headers.end()) {
            env_vars_vec.push_back("CONTENT_TYPE=" + it_type->second);
        } else {
            env_vars_vec.push_back("CONTENT_TYPE="); // Default empty
        }

        std::map<std::string, std::string>::const_iterator it_len = _request.headers.find("content-length");
        if (it_len != _request.headers.end()) {
            env_vars_vec.push_back("CONTENT_LENGTH=" + it_len->second);
        } else {
            env_vars_vec.push_back("CONTENT_LENGTH=0"); // Default 0
        }
    } else { // For GET, ensure these are not present or empty
        env_vars_vec.push_back("CONTENT_TYPE=");
        env_vars_vec.push_back("CONTENT_LENGTH=");
    }

    // Add DOCUMENT_ROOT as it's often required by PHP CGI
    if (_locationConfig && !_locationConfig->root.empty()) {
        std::string doc_root = _locationConfig->root;
        // Ensure DOCUMENT_ROOT does NOT end with a slash, unless it's just "/"
        if (doc_root.length() > 1 && doc_root[doc_root.length() - 1] == '/') {
            doc_root = doc_root.substr(0, doc_root.length() - 1);
        }
        env_vars_vec.push_back("DOCUMENT_ROOT=" + doc_root);
    } else {
        // Fallback for DOCUMENT_ROOT if no location root is defined (should ideally not happen for CGI)
        env_vars_vec.push_back("DOCUMENT_ROOT=/"); // Default to root of filesystem
    }


    // Other HTTP headers (prefixed with HTTP_ and converted to uppercase with _ instead of -)
    for (std::map<std::string, std::string>::const_iterator it = _request.headers.begin(); it != _request.headers.end(); ++it) {
        std::string header_name = it->first;
        // Skip Content-Type and Content-Length as they are handled explicitly above
        if (StringUtils::ciCompare(header_name, "content-type") || StringUtils::ciCompare(header_name, "content-length")) {
            continue;
        }
        std::transform(header_name.begin(), header_name.end(), header_name.begin(), static_cast<int(*)(int)>(std::toupper));
        for (size_t i = 0; i < header_name.length(); ++i) {
            if (header_name[i] == '-') {
                header_name[i] = '_';
            }
        }
        env_vars_vec.push_back("HTTP_" + header_name + "=" + it->second);
    }
    
    // REMOTE_ADDR and REMOTE_PORT (assuming these can be derived from request's client connection)
    // For now, hardcode or assume HttpRequest is extended with these if not present.
    // TODO: Integrate actual client IP/Port if available.
    env_vars_vec.push_back("REMOTE_ADDR=127.0.0.1"); // Placeholder
    env_vars_vec.push_back("REMOTE_PORT=8080"); // Placeholder (or actual client port)

    // Convert std::vector<std::string> to char**
    char** envp = new char*[env_vars_vec.size() + 1];
    for (size_t i = 0; i < env_vars_vec.size(); ++i) {
        envp[i] = new char[env_vars_vec[i].length() + 1];
        std::strcpy(envp[i], env_vars_vec[i].c_str());
    }
    envp[env_vars_vec.size()] = NULL;
    return envp;
}

// --- Private Helper: Create CGI Arguments ---
char** CGIHandler::_createCGIArguments() const {
    // Subject: "Votre programme doit appeler le CGI avec le fichier demandé comme premier argument."
    // This means argv[0] is the executable, argv[1] is the script path.
    // MODIFIED: argv[1] now uses the FULL _cgi_script_path (absolute path)
    // This eliminates the ambiguity php-cgi might have with relative paths after chdir.
    char** argv = new char*[3]; // executable, _cgi_script_path, NULL
    argv[0] = new char[_cgi_executable_path.length() + 1];
    std::strcpy(argv[0], _cgi_executable_path.c_str());
    
    argv[1] = new char[_cgi_script_path.length() + 1]; // Use the full absolute path
    std::strcpy(argv[1], _cgi_script_path.c_str());
    
    argv[2] = NULL;
    return argv;
}

// --- Private Helper: Free char** Arrays ---
void CGIHandler::_freeCGICharArrays(char** arr) const {
    if (arr) {
        for (int i = 0; arr[i] != NULL; ++i) {
            delete[] arr[i];
        }
        delete[] arr;
    }
}

// --- Private Helper: Close Pipe File Descriptors ---
void CGIHandler::_closePipes() {
    if (_fd_stdin[0] != -1) {
        close(_fd_stdin[0]);
        _fd_stdin[0] = -1;
    }
    if (_fd_stdin[1] != -1) {
        close(_fd_stdin[1]);
        _fd_stdin[1] = -1;
    }
    if (_fd_stdout[0] != -1) {
        close(_fd_stdout[0]);
        _fd_stdout[0] = -1;
    }
    if (_fd_stdout[1] != -1) {
        close(_fd_stdout[1]);
        _fd_stdout[1] = -1;
    }
}

// --- Start CGI Process ---
bool CGIHandler::start() {
    if (_state != CGIState::NOT_STARTED) {
        std::cerr << "ERROR: CGI process already started or in an invalid state." << std::endl;
        return false;
    }

    // Check if paths are valid before starting
    if (_cgi_script_path.empty() || _cgi_executable_path.empty()) {
        std::cerr << "ERROR: CGIHandler: Script or executable path not properly initialized." << std::endl;
        _state = CGIState::CGI_PROCESS_ERROR;
        return false;
    }

    // 1. Create pipes
    if (pipe(_fd_stdin) == -1) {
        std::cerr << "ERROR: Failed to create stdin pipe: " << strerror(errno) << std::endl;
        _state = CGIState::FORK_FAILED; // Use FORK_FAILED as a general setup error before fork
        return false;
    }
    if (pipe(_fd_stdout) == -1) {
        std::cerr << "ERROR: Failed to create stdout pipe: " << strerror(errno) << std::endl;
        _closePipes();
        _state = CGIState::FORK_FAILED;
        return false;
    }

    // 2. Set parent's ends of pipes to non-blocking
    if (!_setNonBlocking(_fd_stdin[1]) || !_setNonBlocking(_fd_stdout[0])) {
        _closePipes();
        _state = CGIState::FORK_FAILED;
        return false;
    }

    // 3. Fork process
    _cgi_pid = fork();
    if (_cgi_pid == -1) {
        std::cerr << "ERROR: Failed to fork CGI process: " << strerror(errno) << std::endl;
        _closePipes();
        _state = CGIState::FORK_FAILED;
        return false;
    }

    if (_cgi_pid == 0) { // Child process
        // Close parent's ends of pipes in child
        close(_fd_stdin[1]);
        close(_fd_stdout[0]);

        // Redirect stdin/stdout to pipe ends
        if (dup2(_fd_stdin[0], STDIN_FILENO) == -1) {
            std::cerr << "ERROR: dup2 STDIN_FILENO failed in CGI child: " << strerror(errno) << std::endl;
            _exit(EXIT_FAILURE); // Use _exit in child after fork
        }
        if (dup2(_fd_stdout[1], STDOUT_FILENO) == -1) {
            std::cerr << "ERROR: dup2 STDOUT_FILENO failed in CGI child: " << strerror(errno) << std::endl;
            _exit(EXIT_FAILURE);
        }

        // Close original pipe FDs after dup2
        close(_fd_stdin[0]);
        close(_fd_stdout[1]);

        // Prepare environment and arguments
        char** envp = _createCGIEnvironment();
        char** argv = _createCGIArguments();

        // Execute CGI program
        execve(_cgi_executable_path.c_str(), argv, envp);

        // If execve returns, it must have failed
        std::cerr << "ERROR: execve failed for CGI: " << _cgi_executable_path << " - " << strerror(errno) << std::endl;
        // Clean up memory before exiting (important for child process)
        _freeCGICharArrays(envp);
        _freeCGICharArrays(argv);
        _exit(EXIT_FAILURE);
    } else { // Parent process
        // Close child's ends of pipes in parent
        close(_fd_stdin[0]);
        close(_fd_stdout[1]);

        // Initial state: If POST, need to write body; otherwise, just read output.
        if (_request.method == "POST" && _request_body_ptr && !_request_body_ptr->empty()) {
            _state = CGIState::WRITING_INPUT;
        } else {
            _state = CGIState::READING_OUTPUT;
        }
        std::cout << "DEBUG: CGI process forked with PID " << _cgi_pid << ". Initial state: " << _state << std::endl;
        return true;
    }
}

// --- Getters for File Descriptors ---
int CGIHandler::getReadFd() const {
    return _fd_stdout[0]; // Read from CGI's stdout
}

int CGIHandler::getWriteFd() const {
    return _fd_stdin[1]; // Write to CGI's stdin
}

// --- Handle Reading from CGI (stdout) ---
void CGIHandler::handleRead() {
    // This state allows for reading even if we are still in the process of writing the request body,
    // which can happen if the CGI script starts sending output before all input is received.
    if (_state != CGIState::READING_OUTPUT && _state != CGIState::WRITING_INPUT) {
        std::cerr << "WARNING: handleRead called in unexpected state: " << _state << std::endl;
        return;
    }

    char buffer[4096];
    ssize_t bytes_read = read(_fd_stdout[0], buffer, sizeof(buffer));

    if (bytes_read > 0) {
        _cgi_response_buffer.insert(_cgi_response_buffer.end(), buffer, buffer + bytes_read);
        std::cout << "DEBUG: Read " << bytes_read << " bytes from CGI stdout. Total: " << _cgi_response_buffer.size() << std::endl;
    } else if (bytes_read == 0) { // EOF from CGI stdout
        std::cout << "DEBUG: EOF received from CGI stdout." << std::endl;
        close(_fd_stdout[0]); // Close read end of stdout pipe
        _fd_stdout[0] = -1; // Mark as closed
        
        // After receiving all output, parse it
        _parseCGIOutput();
        // Transition to COMPLETE only if all input was sent (if applicable) and output fully parsed
        if (_state != CGIState::WRITING_INPUT) { // If still writing input, keep that state, wait for all input to be written
            _state = CGIState::COMPLETE;
        } else {
             // If EOF on stdout but still waiting to send stdin, something might be wrong with CGI behavior
             // Or it processed input and returned early. For now, let it transition to COMPLETE if input stream is closed.
             // A more robust server might have a separate state for 'CGI finished but still writing input'
             // or 'CGI finished, waiting for client response send'.
            if (_fd_stdin[1] == -1) { // If stdin pipe is also closed
                _state = CGIState::COMPLETE;
            }
        }
    } else if (bytes_read == -1) { // Error
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "ERROR: Reading from CGI stdout pipe failed: " << strerror(errno) << std::endl;
            _state = CGIState::CGI_PROCESS_ERROR;
            _closePipes();
        }
    }
}

// --- Handle Writing to CGI (stdin) ---
void CGIHandler::handleWrite() {
    if (_state != CGIState::WRITING_INPUT) {
        std::cerr << "WARNING: handleWrite called in unexpected state: " << _state << std::endl;
        return;
    }

    if (!_request_body_ptr || _request_body_ptr->empty()) {
        std::cout << "DEBUG: No request body to write to CGI stdin or all sent." << std::endl;
        close(_fd_stdin[1]); // No more data to send, close pipe to signal EOF to CGI
        _fd_stdin[1] = -1; // Mark as closed
        _state = CGIState::READING_OUTPUT; // Transition to reading CGI output
        return;
    }

    size_t remaining_bytes = _request_body_ptr->size() - _request_body_sent_bytes;
    if (remaining_bytes == 0) {
        std::cout << "DEBUG: All request body already sent to CGI stdin." << std::endl;
        close(_fd_stdin[1]); // All sent, close pipe
        _fd_stdin[1] = -1; // Mark as closed
        _state = CGIState::READING_OUTPUT; // Transition to reading CGI output
        return;
    }

    ssize_t bytes_written = write(_fd_stdin[1],
                                  &(*_request_body_ptr)[_request_body_sent_bytes],
                                  remaining_bytes);

    if (bytes_written > 0) {
        _request_body_sent_bytes += bytes_written;
        std::cout << "DEBUG: Wrote " << bytes_written << " bytes to CGI stdin. Total sent: " << _request_body_sent_bytes << std::endl;
        if (_request_body_sent_bytes == _request_body_ptr->size()) {
            std::cout << "INFO: All request body sent to CGI stdin." << std::endl;
            close(_fd_stdin[1]); // All data sent, close write end to signal EOF to CGI
            _fd_stdin[1] = -1; // Mark as closed
            _state = CGIState::READING_OUTPUT; // Transition to reading CGI output
        }
    } else if (bytes_written == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EPIPE) { // EPIPE can happen if CGI exits early
            std::cerr << "ERROR: Writing to CGI stdin pipe failed: " << strerror(errno) << std::endl;
            _state = CGIState::CGI_PROCESS_ERROR;
            _closePipes();
        }
    }
}

// --- Poll CGI Process Status ---
void CGIHandler::pollCGIProcess() {
    // Only poll if the CGI process ID is valid and it's not in a final state
    if (_cgi_pid != -1 && !isFinished()) { // Rechecked isFinished() inside the while loop in test main.
                                         // It can sometimes enter COMPLETE state inside handleRead or pollCGIProcess itself.
                                         // This check ensures we don't try to waitpid on an already finished process repeatedly.
        int status;
        pid_t result = waitpid(_cgi_pid, &status, WNOHANG);

        if (result == _cgi_pid) { // Child has exited
            if (WIFEXITED(status)) {
                _cgi_exit_status = WEXITSTATUS(status);
                std::cout << "DEBUG: CGI process " << _cgi_pid << " exited with status " << _cgi_exit_status << std::endl;
            } else if (WIFSIGNALED(status)) {
                _cgi_exit_status = WTERMSIG(status);
                std::cerr << "ERROR: CGI process " << _cgi_pid << " terminated by signal " << _cgi_exit_status << std::endl;
                _state = CGIState::CGI_PROCESS_ERROR;
            } else {
                std::cerr << "ERROR: CGI process " << _cgi_pid << " exited abnormally." << std::endl;
                _state = CGIState::CGI_PROCESS_ERROR;
            }

            // After the child exits, ensure all pipes are closed
            _closePipes();

            // If the CGI headers haven't been parsed yet (meaning we didn't get EOF on stdout pipe, or it was an error),
            // try to parse whatever output we have or set an error state.
            if (!_cgi_headers_parsed && !_cgi_response_buffer.empty()) {
                std::cerr << "WARNING: CGI process exited before EOF on stdout, attempting to parse partial output." << std::endl;
                _parseCGIOutput();
            } else if (!_cgi_headers_parsed && _cgi_response_buffer.empty() && _state != CGIState::CGI_PROCESS_ERROR) {
                 // If CGI exited without sending any output and no other error, it's a server error.
                std::cerr << "ERROR: CGI process exited without any output and no headers parsed." << std::endl;
                _final_http_response.setStatus(500);
                _final_http_response.addHeader("Content-Type", "text/html");
                _final_http_response.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI process exited without output.</p></body></html>");
                _state = CGIState::CGI_PROCESS_ERROR;
            }

            // Final state transition based on success/failure
            if (_state != CGIState::CGI_PROCESS_ERROR && _state != CGIState::TIMEOUT) {
                _state = CGIState::COMPLETE;
            }

        } else if (result == -1) { // Error with waitpid call itself
            std::cerr << "ERROR: waitpid failed for CGI process " << _cgi_pid << ": " << strerror(errno) << std::endl;
            _state = CGIState::CGI_PROCESS_ERROR;
            _closePipes();
        }
        // If result == 0, child is still running (WNOHANG)
    }
}

// --- Getters for State and Response ---
CGIState::Type CGIHandler::getState() const {
    return _state;
}

bool CGIHandler::isFinished() const {
    return _state == CGIState::COMPLETE || _state == CGIState::TIMEOUT ||
           _state == CGIState::CGI_PROCESS_ERROR || _state == CGIState::FORK_FAILED;
}

const HttpResponse& CGIHandler::getHttpResponse() const {
    return _final_http_response;
}

pid_t CGIHandler::getCGIPid() const {
    return _cgi_pid;
}

void CGIHandler::setTimeout() {
    if (isFinished()) return; // Already finished or errored

    std::cerr << "WARNING: CGI process " << _cgi_pid << " timed out." << std::endl;
    _state = CGIState::TIMEOUT;
    // Attempt to kill the CGI process if it's still running
    if (_cgi_pid != -1) {
        kill(_cgi_pid, SIGTERM); // Send termination signal
    }
    _closePipes();

    // Generate a 504 Gateway Timeout response
    _final_http_response.setStatus(504); // 504 Gateway Timeout
    _final_http_response.addHeader("Content-Type", "text/html");
    _final_http_response.setBody("<html><body><h1>504 Gateway Timeout</h1><p>The CGI script did not respond in time.</p></body></html>");
}

// --- Private Helper: Parse CGI Output ---
void CGIHandler::_parseCGIOutput() {
    if (_cgi_headers_parsed) { // Avoid re-parsing
        return;
    }

    std::string raw_output(_cgi_response_buffer.begin(), _cgi_response_buffer.end());
    size_t header_end_pos = raw_output.find("\r\n\r\n");

    bool crlf_crlf = true;
    if (header_end_pos == std::string::npos) {
        header_end_pos = raw_output.find("\n\n");
        crlf_crlf = false;
    }

    std::string cgi_headers_str;
    std::string cgi_body_str;

    if (header_end_pos != std::string::npos) {
        cgi_headers_str = raw_output.substr(0, header_end_pos);
        cgi_body_str = raw_output.substr(header_end_pos + (crlf_crlf ? 4 : 2));
    } else {
        std::cerr << "WARNING: No double CRLF/LF found in CGI output, treating entire output as body (CGI output was: " << raw_output.substr(0, std::min((size_t)200, raw_output.length())) << "..." << std::endl;
        cgi_body_str = raw_output;
    }

    _final_http_response.setBody(cgi_body_str); // Set the body first

    std::istringstream header_stream(cgi_headers_str);
    std::string line;
    int cgi_status_code = 200; // Default CGI status
    bool content_type_provided_by_cgi = false; // Flag to track if Content-Type was explicitly provided

    while (std::getline(header_stream, line) && !line.empty()) {
        std::string trimmed_line = line;
        StringUtils::trim(trimmed_line);

        size_t colon_pos = trimmed_line.find(':');
        if (colon_pos == std::string::npos) {
            std::cerr << "WARNING: Malformed CGI header line (no colon): " << trimmed_line << std::endl;
            continue;
        }

        std::string name = trimmed_line.substr(0, colon_pos);
        StringUtils::trim(name);

        std::string value = trimmed_line.substr(colon_pos + 1);
        StringUtils::trim(value);

        if (StringUtils::ciCompare(name, "Status")) {
            std::istringstream status_stream(value);
            status_stream >> cgi_status_code;
            if (status_stream.fail()) {
                std::cerr << "WARNING: Failed to parse CGI Status code from '" << value << "'. Defaulting to 200." << std::endl;
                cgi_status_code = 200;
            }
            _final_http_response.setStatus(cgi_status_code);
        } else if (StringUtils::ciCompare(name, "Content-Type")) {
            // Normalize to "Content-Type" and add to response
            _final_http_response.addHeader("Content-Type", value);
            content_type_provided_by_cgi = true;
        } else {
            // For other headers, add them as they are
            _final_http_response.addHeader(name, value);
        }
    }
    
    // If Content-Length was not provided by CGI, but we have a body, calculate it.
    if (_final_http_response.getHeaders().find("Content-Length") == _final_http_response.getHeaders().end()) {
        _final_http_response.addHeader("Content-Length", StringUtils::longToString(_final_http_response.getBody().size()));
    }

    // Default Content-Type ONLY if not provided by CGI
    if (!content_type_provided_by_cgi) {
        _final_http_response.addHeader("Content-Type", "application/octet-stream"); // Safe default
        std::cerr << "WARNING: CGI did not provide Content-Type, defaulting to application/octet-stream." << std::endl;
    }

    _cgi_headers_parsed = true;
    if (_state != CGIState::COMPLETE) {
        _state = CGIState::PROCESSING_OUTPUT;
    }
}
