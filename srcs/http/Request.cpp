#include "Request.hpp"
#include "webserv.hpp"
#include <sstream>
#include <string>

Request::Request() {
    _unparsed_request = "";
    _raw_body = "";
    _request_method = NOT_SET;
    _body_bytes_read = 0;
    _resource = "";
}

Request::~Request() {
}

void	Request::setStatusCode(StatusCode status_code) {
	_status_code = status_code;
}

// Getters
std::string     Request::getUnparsedRequest(void) {
    return _unparsed_request;
}

std::string     Request::getRawStartline(void) {
    return (_raw_start_line);
}

std::string     Request::getRawHeaders(void) {
    return (_raw_headers);
}


std::string     Request::getRawBody(void) {
    return (_raw_body);
}


RequestMethod   Request::getRequestMethod(void) {
    return (_request_method);
}

Uri&             Request::getUri(void) {
     return (_uri);
}


std::map<std::string, std::string>  Request::getHeaders(void) {
    return (_headers);
}

StatusCode		Request::getStatusCode(void) {
	return _status_code;
}

std::string     Request::getResource(void) {
    return _resource;
}

bool     Request::headersFullyParsed() {
    if (_unparsed_request.find("\r\n\r\n") != std::string::npos)
        return (true);
    else
        return (false);
}

bool     Request::isFullyParsed() {
    if (headersFullyParsed() && !_has_body)
        return (true);
    else if (headersFullyParsed() && _has_body && _raw_body.length() == _body_length)
    {
        return (true);
    }
    else
        return (false);
}

int     Request::parseRequest(char *buf, int bytes_read, ServerBlock* sb) {//A MODIF 
    int         start = bytes_read;

    /*
    *   Read in a request byte per byte. Part of the request could be in the next/previous call to send(2)
    *   of the main server loop. For this reason we keep adding into the unparsed request until
    *   there is a fully parsed request, or until a Content-Length body has been received.
    */
    while (start < BUFF_SIZE)
    {
        // First check if all the headers of the current request are parsed!
        if (!headersFullyParsed())
        {
            // std::cout << "currently reading byte i: " << start << std::endl;
            _unparsed_request += buf[start];
            if (_unparsed_request.find("\r\n\r\n") != std::string::npos) // let's say we found this at byte 200
            {
                // we should now have complete headers in _unparsed_request?
                parseRequestStartLine(sb);
                parseRequestHeaders();
                printRequest();
            }
            bytes_read++;
        }
        // If there is a body to parse, execute below code
        else
        {
            if (_has_body)
            {
                _raw_body += buf[start];
                _body_bytes_read++;
                // std::cout << "current body_bytes_read is" << _body_bytes_read << std::endl;
                bytes_read++;
                if (static_cast<unsigned long>(_body_bytes_read) == _body_length) // keep reading until we have the entire body
                {
					if (_body_length > sb->getClientMaxBodySize())
						throw Request::ContentTooLargeException();
                    break ;
                }
            }
        }
        start++;
    }
    return (bytes_read);
}

std::string trim_slashes(std::string str) {
    size_t first = str.find_first_not_of('/');
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of('/');
    return str.substr(first, last - first + 1);
}

LocationBlock* locationExistsInBlock(std::vector<LocationBlock*>& lbs, std::string url) {
    for (std::vector<LocationBlock*>::iterator it = lbs.begin(); it != lbs.end(); it++)
    {
       if ((*it)->getURL() == url)
        return (*it);
    }
    // return NULL if it doesnt exist
    return (NULL);
}

void    Request::parseRequestStartLine(ServerBlock* sb) {// A REFAIRE
    std::vector<std::string> startline_split;

    _raw_start_line = _unparsed_request.substr(0, _unparsed_request.find("\r\n"));
    std::string keys[] = {"GET", "POST", "DELETE"};
    std::size_t         pos = _raw_start_line.length();
    for (int i = 0; i < 3; i++)//check request to use
    {
        std::size_t keyPos = _raw_start_line.find(keys[i]);
        if (keyPos != std::string::npos && keyPos < pos)
            pos = keyPos;
    }
    if (pos != _raw_start_line.length())
        _raw_start_line = _raw_start_line.substr(pos);

    _unparsed_request = _unparsed_request.substr(_unparsed_request.find("\r\n") + 2);

    startline_split = ft_split(_raw_start_line, ' ');
    if (startline_split.size() != 3)
        throw BadRequestException();

    if (startline_split[0] == "GET")
        _request_method = GET;
    else if (startline_split[0] == "POST")
        _request_method = POST;
    else if (startline_split[0] == "DELETE")
        _request_method = DELETE;
    else
        throw ("Plese use GET, POST or DELETE only...");
    if ((_request_method == GET && !(sb->_GET_allowed)) || (_request_method == POST && !(sb->_POST_allowed)) || (_request_method == DELETE && !(sb->_DELETE_allowed)))
        throw ("We havent autorisation for use this method");

    parseURI(startline_split[1]);
    if (is_subdirectory(sb->getRootFolder(), trim_slashes(_uri.getPath())))
    {
        if (trim_slashes(_uri.getPath()) == "images")
            throw ("Path incorrect");
        if (!locationExistsInBlock(sb->getLocationBlocks(), trim_slashes(_uri.getPath())))
            throw ("We havent autorisation for that");
        else
        {
            std::string rootFolder = locationExistsInBlock(sb->getLocationBlocks(), trim_slashes(_uri.getPath()))->getRootFolder();
            std::string path = trim_slashes(_uri.getPath());
            std::string index =locationExistsInBlock(sb->getLocationBlocks(), trim_slashes(_uri.getPath()))->getIndexPage();
            _resource = rootFolder + "/" + path + "/" + index;
        }
    }
    else if (startline_split[1] == "/")
        _resource = sb->getRootFolder() + _uri.getPath() + sb->getIndexPage();
    else if (hasFileExtension(_uri.getPath(), ".php"))
        _resource = _uri.getPath();
    else
        _resource = sb->getRootFolder() + _uri.getPath();
    if (!ft_is_resource_available(_resource) && !hasFileExtension(_uri.getPath(), ".php"))
        throw ("invalide path, sorry please check is correct");
    if (startline_split[2] != "HTTP/1.1")
        throw ("Please use HTTP 1.1v");
}

void    Request::parseRequestHeaders() {// A REFAIRE : c du COPIE COLLE
    std::string         line;
    _raw_headers = _unparsed_request.substr(0, _unparsed_request.find("\r\n\r\n"));
    std::stringstream   ss(_raw_headers);

    while (std::getline(ss, line, '\r'))
    {
        std::stringstream   ls(line);
        std::string         key;
        std::string         value;

        std::getline(ls, key, ':');
        key.erase(key.begin(), std::find_if(key.begin(), key.end(), ft_is_non_whitespace));
        key.erase(std::find_if(key.rbegin(), key.rend(), ft_is_non_whitespace).base(), key.end());

        std::getline(ls, value);
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), ft_is_non_whitespace));
        value.erase(std::find_if(value.rbegin(), value.rend(), ft_is_non_whitespace).base(), value.end());
        _headers.insert(std::make_pair(key, value));
    }
    std::map<std::string, std::string>::iterator it = _headers.find("Content-Length");
    if (it != _headers.end())
    {
        _has_body = true;
        _body_length = std::strtoul(it->second.c_str(), NULL, 0);
    }
}

void    Request::parseRequestBody(void) {
    _raw_body += _unparsed_request;
}

void    Request::parseUri(std::string uri) {
    _uri.setPath(uri.substr(0, uri.find('?')));
    if (uri.find('?') != std::string::npos)
        _uri.setQuery(uri.substr(uri.find('?') + 1, (uri.find('#'))));
}

void    Request::printRequest(void)
{
    std::cout << "Requete Now : " << _raw_start_line << " - "<< _raw_headers << std::endl;
}