/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   token.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/02 17:22:40 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/09 18:31:44 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef TOKEN_HPP
# define TOKEN_HPP

# include <string>

typedef enum tokenType {
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
	T_NUMBER				// integers, ip:port ("80", "10")
} tokenType;

typedef struct token {
	tokenType   type;
	std::string value;
	int         line, column;

	token(tokenType type, std::string value, int line, int column)
		: type(type), value(value), line(line), column(column)
		{}
} token;

const std::string       tokenTypeToString(tokenType type);

#endif