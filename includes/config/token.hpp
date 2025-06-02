/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   token.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/02 17:22:40 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/02 17:32:29 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef TOKEN_HPP
# define TOKEN_HPP

# include <string>

enum TokenType {
	// End / errors
	T_EOF,				// End of file
	T_ERROR,			// Unexpected char / word

	// Structure symbols
	T_LBRACE,				// '{'
	T_RBRACE,				// '}'
	T_SEMICOLON,			// ';'

	// Location modifiers
	T_EQ,					// '='
	T_TILDE,				// '~'
	T_TILDE_STAR,			// '~*'
	T_CARET_TILDE,			// '^~'

	// Directives (keywords)
	T_SERVER,				// "server"
	T_LISTEN,				// "listen"
	T_SERVER_NAME,			// "server_name"
	T_ERROR_PAGE,			// "error_page"
	T_CLIENT_MAX_BODY,		// "client_max_body_size"
	T_INDEX,				// "index"
	T_CGI_EXTENSION,		// "cgi_extension"
	T_CGI_PATH,				// "cgi_path"
	T_ALLOWED_METHODS,		// "allowed_methods"
	T_RETURN,				// "return"
	T_ROOT,					// "root"
	T_AUTOINDEX,			// "autoindex"
	T_UPLOAD_ENABLED,		// "upload_enabled"
	T_UPLOAD_STORE,			// "upload_store"
	T_LOCATION,				// "location"
	T_ERROR_LOG,			// "error_log"

	// Other data/values
	T_IDENTIFIER,			// strings/words that are not keywords specified above
	T_STRING,				// string "..." or '...'
	T_NUMBER				// integers (ex. "80", "10")
};

struct Token {
	TokenType   type;
	std::string text;
	int         line, column;

	Token(TokenType type, std::string value, int line, int column)
		: type(type), text(value), line(line), column(column)
		{}
};

#endif