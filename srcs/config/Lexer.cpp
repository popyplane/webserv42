/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/02 10:47:25 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/11 23:49:44 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/config/Lexer.hpp"

bool    readFile(const std::string &fileName, std::string &out)
{
	std::ifstream   file(fileName);
	std::string     buffer;

	if (!file)
		return false;
	while (std::getline(file, buffer))
		out += buffer + "\n";
	return true;
}


LexerError::LexerError(const std::string& msg, int line, int col) : std::runtime_error(msg), _line(line), _col(col)
{ }
	
int LexerError::getLine() const
{ return (_line); }
	
int LexerError::getColumn() const
{ return (_col); }

Lexer::Lexer(const std::string &input) : _input(input), _pos(0), _line(1), _column(1)
{}

Lexer::~Lexer()
{}

bool    Lexer::isAtEnd() const
{ return (_pos >= _input.size()); }

char	Lexer::peek() const
{ return (isAtEnd() ? '\0' : _input[_pos]); }

char	Lexer::get()
{
	char	c = _input[_pos++];

	if (c == '\n') {
		++_line;
		_column = 1;
	} else {
		++_column;
	}
	return (c);
}

void	Lexer::skipWhitespaceAndComments()
{
	while (!isAtEnd()) {
		char	c = peek();

		if (std::isspace(c)) {
			get();
		} else if (c == '#'){
			while (!isAtEnd() && peek() != '\n')
				get();
		} else {
			break;
		}
	}
}

token	Lexer::nextToken()
{
	skipWhitespaceAndComments();
	if (isAtEnd())
		return (token(T_EOF, "", _line, _column + 1));

	char	curr = peek();
	
	if (curr == '=' || curr == '~' || curr == '^')
		return tokeniseModifier();
	if (curr == '{' || curr == '}' || curr == ';')
		return tokeniseSymbol();
	if (curr == '"' || curr == '\'')
		return tokeniseString();
	if (std::isalpha(curr) || curr == '_' || curr == '.' || curr == '-' || curr == '/' || curr == '$')
		return (tokeniseIdentifier());
	if (std::isdigit(curr))
		return (tokeniseNumber());
	
	std::ostringstream oss;
	
	oss	<< "Unxpected char : '" << get() << "' from Lexer::nextToken()";
	error(oss.str());
}

token	Lexer::tokeniseModifier()
{
	int		startLn = _line, startCol = _column;
	char	c = get();

	if (c == '=')						// case =
		return (token(T_EQ, "=", startLn, startCol));
	if (c == '~') {						// case ~ or ~*
		if (peek() == '*') {
			get();
			return (token(T_TILDE_STAR, "~*", startLn, startCol));
		}
		return (token(T_TILDE, "~", startLn, startCol));
	}
	if (c == '^') {						// case ^~
		if (peek() == '~') {
			get();
			return (token(T_CARET_TILDE, "^~", startLn, startCol));
		}
	}

	// must never happen
	return (token(T_ERROR, "Invalid modifier", startLn, startCol));
}

token	Lexer::tokeniseSymbol()
{
	int			startLn = _line, startCol = _column;
	char	c = get();

	if (c == '{')
		return (token(T_LBRACE, "{", startLn, startCol));
	if (c == '}')
		return (token(T_RBRACE, "}", startLn, startCol));
	if (c == ';')
		return (token(T_SEMICOLON, ";", startLn, startCol));

	// must never happen >> if we want to handle other symbol as error
	return token(T_ERROR, std::string("Unknown symbol: ") + c, startLn, startCol);		// check if we can put ("..." + c)
}

token	Lexer::tokeniseString()
{
	int			startLn = _line, startCol = _column;
	char		quote = get();
	std::string	buffer;

	while (!isAtEnd() && peek() != quote) {
		if (peek() == '\\') {
			get();
			if (!isAtEnd())
				buffer += get();
		} else {
			buffer += get();
		}
	}
	if (peek() == quote) {
		get();
		return (token(T_STRING, buffer, startLn, startCol));
	}

	// case where the quote is never closed
	return (token(T_ERROR, "Unterminated string", startLn, startCol));
}

token	Lexer::tokeniseNumber()
{
	std::string	buffer;
	int			startLn = _line, startCol = _column;

	
	while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.' || peek() == ':' || peek() == 'K' || peek() == 'M' || peek() == 'G'))
		buffer += get();

	return (token(T_NUMBER, buffer, startLn, startCol));
}

token Lexer::tokeniseIdentifier()
{
	std::string buffer;
	int			startLn = _line, startCol = _column;

	while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_' || peek() == '.'
						|| peek() == '-' || peek() == ':' || peek() == '/' || peek() == '$'))
		buffer += get();

	if (buffer == "server")					return (token(T_SERVER, buffer, startLn, startCol));
	if (buffer == "listen")					return (token(T_LISTEN, buffer, startLn, startCol));
	if (buffer == "server_name")			return (token(T_SERVER_NAME, buffer, startLn, startCol));
	if (buffer == "error_page")				return (token(T_ERROR_PAGE, buffer, startLn, startCol));
	if (buffer == "client_max_body_size")	return (token(T_CLIENT_MAX_BODY, buffer, startLn, startCol));
	if (buffer == "index")					return (token(T_INDEX, buffer, startLn, startCol));
	if (buffer == "cgi_extension")			return (token(T_CGI_EXTENSION, buffer, startLn, startCol));
	if (buffer == "cgi_path")				return (token(T_CGI_PATH, buffer, startLn, startCol));
	if (buffer == "allowed_methods")		return (token(T_ALLOWED_METHODS, buffer, startLn, startCol));
	if (buffer == "return")					return (token(T_RETURN, buffer, startLn, startCol));
	if (buffer == "root")					return (token(T_ROOT, buffer, startLn, startCol));
	if (buffer == "autoindex")				return (token(T_AUTOINDEX, buffer, startLn, startCol));
	if (buffer == "upload_enabled")			return (token(T_UPLOAD_ENABLED, buffer, startLn, startCol));
	if (buffer == "upload_store")			return (token(T_UPLOAD_STORE, buffer, startLn, startCol));
	if (buffer == "location")				return (token(T_LOCATION, buffer, startLn, startCol));
	if (buffer == "error_log")				return (token(T_ERROR_LOG, buffer, startLn, startCol));

	// Ohter generic values
	return (token(T_IDENTIFIER, buffer, startLn, startCol));
}

void	Lexer::lexConf()
{
	token	t = nextToken();

	while (t.type != T_EOF) {
		_tokens.push_back(t);
		t = nextToken();
	}
	_tokens.push_back(t);
}

void	Lexer::dumpTokens()
{
	for (size_t i = 0; i < _tokens.size(); i++) {
		std::cout	<< tokenTypeToString(_tokens[i].type) << " : ["
					<< _tokens[i].value << "] "
					<< "Ln " << _tokens[i].line
					<< ", Col " << _tokens[i].column
					<< std::endl;
	}
}

std::vector<token>	Lexer::getTokens() const
{ return (_tokens); }

void	Lexer::error(const std::string& msg) const
{ throw (LexerError(msg, _line, _column + 1)); }