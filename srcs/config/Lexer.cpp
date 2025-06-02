/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/02 10:47:25 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/02 18:55:14 by baptistevie      ###   ########.fr       */
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

bool    Lexer::isAtEnd() const
{ return (_pos >= _input.size()); }

char	Lexer::peek() const
{ return (isAtEnd() ? '\0' : _input[_pos]); }

char	Lexer::get()
{
	char	c = _input[_pos++];

	++_column;
	if (c == '\n') {
		++_line;
		_column = 0;
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

Token	Lexer::nextToken()
{
	skipWhitespaceAndComments();
	if (isAtEnd())
		return (Token(T_EOF, "", _line, _column));

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
	if (curr == '"' || curr == '\'')
		return (tokeniseString());
	
	char	bad = get();
	return (Token(T_ERROR, std::string("Unexpected: ") + bad, _line, _column - 1));

}

Token	Lexer::tokeniseModifier()
{
	char	c = get();

	if (c == '=')						// case =
		return (Token(T_EQ, "=", _line, _column - 1));
	if (c == '~') {						// case ~ or ~*
		if (peek() == '*') {
			get();
			return (Token(T_TILDE_STAR, "~*", _line, _column - 2));
		}
		return (Token(T_TILDE, "~", _line, _column - 1));
	}
	if (c == '^') {						// case ^~
		if (peek() == '~') {
			get;
			return (Token(T_CARET_TILDE, "^~", _line, _column - 2));
		}
	}

	// must never happen
	return (Token(T_ERROR, "Invalid modifier", _line, _column - 1));
}

Token	Lexer::tokeniseSymbol()
{
	char	c = get();

	if (c == '{')
		return (Token(T_LBRACE, "{", _line, _column - 1));
	if (c == '}')
		return (Token(T_RBRACE, "}", _line, _column - 1));
	if (c == ';')
		return (Token(T_SEMICOLON, ";", _line, _column - 1));

	// must never happen >> if we want to handle other symbol as error
	return Token(T_ERROR, std::string("Unknown symbol: ") + c, _line, _column - 1);		// check if we can put ("..." + c)
}

Token	Lexer::tokeniseString()
{
	char		quote = get();
	std::string	buffer;
	int			backslashNb = 0;

	while (!isAtEnd() && peek() != quote) {
		if (peek() == '\\') {
			get();
			++backslashNb;
			if (!isAtEnd())
				buffer += get();
		} else {
			buffer += get();
		}
	}
	if (peek() == quote) {
		get();
		return (Token(T_STRING, buffer, _line, _column - buffer.size() - backslashNb - 2));
	}

	// case where the quote is never closed
	return (Token(T_ERROR, "Unterminated string", _line, _column - buffer.size() - backslashNb - 1));
}

Token	Lexer::tokeniseNumber()
{
	std::string	buffer;
	
	while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.' || peek() == ':'))
		buffer += get();

	return (Token(T_NUMBER, buffer, _line, _column - buffer.size()));
}

Token Lexer::tokeniseIdentifier()
{
	std::string buffer;

	while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_' || peek() == '.'
						|| peek() == '-' || peek() == ':' || peek() == '/' || peek() == '$'))
		buffer += get();

	if (buffer == "server")					return Token(T_SERVER, buffer, _line, _column - buffer.size());
	if (buffer == "listen")					return Token(T_LISTEN, buffer, _line, _column - buffer.size());
	if (buffer == "server_name")			return Token(T_SERVER_NAME, buffer, _line, _column - buffer.size());
	if (buffer == "error_page")				return Token(T_ERROR_PAGE, buffer, _line, _column - buffer.size());
	if (buffer == "client_max_body_size")	return Token(T_CLIENT_MAX_BODY, buffer, _line, _column - buffer.size());
	if (buffer == "index")					return Token(T_INDEX, buffer, _line, _column - buffer.size());
	if (buffer == "cgi_extension")			return Token(T_CGI_EXTENSION, buffer, _line, _column - buffer.size());
	if (buffer == "cgi_path")				return Token(T_CGI_PATH, buffer, _line, _column - buffer.size());
	if (buffer == "allowed_methods")		return Token(T_ALLOWED_METHODS, buffer, _line, _column - buffer.size());
	if (buffer == "return")					return Token(T_RETURN, buffer, _line, _column - buffer.size());
	if (buffer == "root")					return Token(T_ROOT, buffer, _line, _column - buffer.size());
	if (buffer == "autoindex")				return Token(T_AUTOINDEX, buffer, _line, _column - buffer.size());
	if (buffer == "upload_enabled")			return Token(T_UPLOAD_ENABLED, buffer, _line, _column - buffer.size());
	if (buffer == "upload_store")			return Token(T_UPLOAD_STORE, buffer, _line, _column - buffer.size());
	if (buffer == "location")				return Token(T_LOCATION, buffer, _line, _column - buffer.size());
	if (buffer == "error_log")				return Token(T_ERROR_LOG, buffer, _line, _column - buffer.size());

	// Ohter generic values
	return Token(T_IDENTIFIER, buffer, _line, _column - buffer.size());
}

