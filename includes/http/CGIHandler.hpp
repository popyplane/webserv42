/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/25 17:40:21 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/25 17:45:03 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <unistd.h>     // For fork, pipe, dup2, execve, close
#include <sys/wait.h>   // For waitpid
#include <fcntl.h>      // For fcntl, O_NONBLOCK
#include <sstream>      // For std::ostringstream
#include <algorithm>    // For std::transform (tolower)
#include <cstdio>       // For remove (unlink)
#include <errno.h>      // For errno and strerror
#include <cstring>      // For strerror (C-style string manipulation)

// Forward declarations to avoid circular dependencies and include actual full headers later
class HttpRequest;
struct ServerConfig;
struct LocationConfig;

#include "HttpResponse.hpp"

// Enum to define the internal state of the CGI process within the handler
namespace CGIState {
    enum Type {
        NOT_STARTED,        // CGI process not yet forked
        FORK_FAILED,        // Fork operation failed
        WRITING_INPUT,      // Currently writing request body to CGI's stdin pipe
        READING_OUTPUT,     // Currently reading CGI's stdout pipe
        PROCESSING_OUTPUT,  // All output read, now parsing headers/body
        COMPLETE,           // CGI process finished, output parsed, ready to build final HTTP response
        TIMEOUT,            // CGI process timed out
        CGI_EXEC_FAILED,    // execve failed in child process
        CGI_PROCESS_ERROR   // Generic error during CGI process I/O or termination
    };
}

class CGIHandler {
public:
    // Constructor: Takes pointers to the request and matched configuration
    CGIHandler(const HttpRequest& request,
               const ServerConfig* serverConfig,
               const LocationConfig* locationConfig);

    // Destructor: Cleans up any open file descriptors and child processes
    ~CGIHandler();

    // Initiates the CGI process (fork, pipe, execve).
    // Returns true if successful in starting, false on immediate failure (e.g., fork error).
    bool start();

    // Returns the read file descriptor for the CGI's stdout pipe.
    // This FD should be monitored by the main event loop for readability.
    int getReadFd() const;

    // Returns the write file descriptor for the CGI's stdin pipe.
    // This FD should be monitored by the main event loop for writability if sending a POST body.
    int getWriteFd() const;

    // Handles incoming data from the CGI's stdout pipe.
    // Called by the main event loop when getReadFd() is readable.
    void handleRead();

    // Handles sending data to the CGI's stdin pipe (for POST requests).
    // Called by the main event loop when getWriteFd() is writable.
    void handleWrite();

    // Checks the status of the CGI child process (non-blocking waitpid).
    // Updates internal state based on process termination.
    void pollCGIProcess();

    // Returns the current state of the CGI execution.
    CGIState::Type getState() const;

    // Checks if the CGI execution has completed and its response is ready.
    bool isFinished() const;

    // Returns the HTTP response generated from the CGI output.
    // Only call this when isFinished() returns true.
    const HttpResponse& getHttpResponse() const;

    // Returns the PID of the forked CGI process.
    pid_t getCGIPid() const;

    // Sets a flag to indicate if a timeout has occurred.
    void setTimeout();

private:
    // Disallow copy constructor and assignment operator for safety with file descriptors and PIDs
    CGIHandler(const CGIHandler& other);
    CGIHandler& operator=(const CGIHandler& other);

    // Internal helper to set a file descriptor to non-blocking mode.
    bool _setNonBlocking(int fd);

    // Internal helper to create environment variables for CGI execution.
    // Returns char** array for execve, needs to be freed later.
    char** _createCGIEnvironment() const;

    // Internal helper to create argument list for execve.
    // Returns char** array, needs to be freed later.
    char** _createCGIArguments() const;

    // Internal helper to free char** arrays (for envp and argv).
    void _freeCGICharArrays(char** arr) const;

    // Parses the CGI's raw output (accumulated in _cgi_response_buffer)
    // into headers and body, building the _final_http_response.
    void _parseCGIOutput();

    // Cleans up CGI related file descriptors.
    void _closePipes();

    const HttpRequest& _request;            // Reference to the original HTTP request
    const ServerConfig* _serverConfig;      // Pointer to the matched server config
    const LocationConfig* _locationConfig;  // Pointer to the matched location config

    pid_t           _cgi_pid;               // PID of the forked CGI child process
    int             _fd_stdin[2];           // Pipe for server->CGI stdin (write to 1, read from 0)
    int             _fd_stdout[2];          // Pipe for CGI->server stdout (write to 1, read from 0)

    std::vector<char> _cgi_response_buffer; // Buffer to accumulate raw CGI output
    size_t          _request_body_sent_bytes; // Tracks how much POST body has been sent
    const std::vector<char>* _request_body_ptr; // Pointer to the request's body data

    CGIState::Type  _state;                 // Current state of the CGI execution
    HttpResponse    _final_http_response;   // The HTTP response built from CGI output
    bool            _cgi_headers_parsed;    // Flag if CGI's HTTP headers have been parsed
    int             _cgi_exit_status;       // Exit status of the CGI child process

    std::string     _cgi_script_path;       // Full file system path to the CGI script
    std::string     _cgi_executable_path;   // Full file system path to the CGI interpreter (e.g., php-cgi)
};

#endif // CGIHANDLER_HPP
