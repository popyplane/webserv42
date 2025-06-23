#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "Request.hpp"
# include "Uri.hpp"
# include "divers.hpp"
# include <string>

class Response {
    protected:
        // to construct for sending
        std::string                         _raw_status_line;
        std::string                         _raw_headers;
        std::string                         _raw_body;
        std::string                         _raw_response;

        // contents
        std::string                         _http_version;
        StatusCode                          _status_code;
        std::string                         _status_string;
        std::map<std::string, std::string>  _headers;
        std::string                         _resource;

        // functions to set headers
        void    setRawHeaders(void);
        void    setDateHeader(void);
        void    setContentLengthHeader();
        void    setConnectionHeader(std::string type);
        void    setContentTypeHeader();
        void    setCacheControl(const char* type);
        void    setRetryAfter(int sec);
		void	setHost(const char* server_name);

        void    setRawResponse(void);
        void    setResource(Request& req);

    public:
        // Constructors
        Response();
		virtual ~Response();
		Response(const Response& src);

        // Public methods
        virtual void    constructResponse(Request& req) = 0;
        virtual void    constructDefaultResponseWithBody(Request& req, std::string raw_body) = 0;
        virtual void    constructConfigResponse(Request& req, std::string filePath) = 0;
        virtual void    setHeaders() = 0;

        std::string     getRawResponse(void);
        std::string     getRawResponse2(void);
        void            printResponse(void);
        void            setStatusCode(StatusCode sc);
};