#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "Uri.hpp"
# include "divers.hpp"
# include "Config.hpp"
#include <string>

typedef enum RequestMethod {
    GET = 0,
    POST,
    DELETE,
    NOT_SET
} RequestMethod;

class Request {
    private:
        // Parsing
        std::string			_unparsed_request;		// Request text that hasn't been analyzed
	    std::string			_raw_start_line; 		// The complete request line such as: `GET / HTTP/1.1`
	    std::string			_raw_headers;           // Raw headers (general headers, response/request headers, entity headers)
	    std::string			_raw_body;              // HTTP Message Body
        bool                _has_body;
        unsigned long       _body_length;
        int                 _body_bytes_read;
		StatusCode			_status_code;

        RequestMethod                       _request_method;
        Uri                                 _uri;
        std::map<std::string, std::string>  _headers;
        std::string                         _resource;

        void                parseRequestStartLine(ServerBlock* sb);
        void                parseRequestHeaders();
        void                parseRequestBody(void);
        void                parseUri(std::string uri);

    public:
        std::string         parse_status;
        Request();
		virtual ~Request();

        std::string     					getUnparsedRequest(void);
        std::string     					getRawStartline(void);
        std::string    						getRawHeaders(void);
        std::string    						getRawBody(void);
        RequestMethod						getRequestMethod(void);
        URI&            					getUri(void);
        std::map<std::string, std::string>  getHeaders(void);
		StatusCode							getStatusCode(void);
        std::string                         getResource(void);

		void	setStatusCode(StatusCode status_code);

        int                 parseRequest(char buf[BUFF_SIZE], int bytes, ServerBlock* sb);
        void                printRequest(void);
        bool                headersFullyParsed(void);
        bool                isFullyParsed(void);

};

#endif