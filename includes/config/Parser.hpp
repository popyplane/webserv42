/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/07 19:19:02 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/09 18:34:05 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
# define PARSER_HPP

# include <sstream>
# include <iostream>
# include <string>
# include <vector>

# include "token.hpp"
# include "Lexer.hpp"
# include "ASTnode.hpp"

class ParseError : public std::runtime_error {
	private:
		int _line, _col;

	public:
		ParseError(const std::string& msg, int line, int col);
		
		int getLine() const;
		int getColumn() const;
};

class Parser {
	private :
		std::vector<token>	_tokens;
		size_t				_current;

		// token management
		token		peek(int offset) const;	// peek current token + offset
		token		consume();				// consume and return current token
		bool		isAtEnd() const;
		bool		checkCurrentType(tokenType type) const;
		token		expectToken(tokenType type, const std::string& context);	// consume expected or throw

		// parsing methods
		std::vector<ASTnode *>		parseConfig();
		BlockNode *					parseServerBlock();
		BlockNode *					parseLocationBlock();
		DirectiveNode *				parseDirective();
		std::vector<std::string>	parseArgs();
		std::string					parseModifier();

		bool						isValidDirective(const std::string& name, const std::string& context) const;
		bool						isModifier(tokenType type) const;

		// error management
		void		error(const std::string& msg) const;
		void		unexpectedToken(const std::string& expected) const;

	public :
		Parser(const std::vector<token>& tokens);
		~Parser();
	
		// main function
		std::vector<ASTnode*> parse();
	
		// cleanup
		void cleanupAST(std::vector<ASTnode*>& nodes);
};

#endif