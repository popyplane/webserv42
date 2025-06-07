/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/28 15:56:14 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/03 16:58:12 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_HPP
# define LEXER_HPP

# include <string>
# include <fstream>
# include <iostream>
# include <cctype>
# include "token.hpp"

bool	readFile(const std::string &fileName, std::string &out);

class Lexer
{
	private:
		const std::string&	_input;			// one-string config file
		size_t				_pos;
		int					_line, _column;	// error management	
		std::vector<token>	_tokens;

		token	nextToken();	// lex the next token
		void	skipWhitespaceAndComments();
		token	tokeniseIdentifier();
		token	tokeniseNumber();
		token	tokeniseString();
		token	tokeniseSymbol();
		token	tokeniseModifier();
		char	peek() const;	// check current char
		char	get();			// consume and return current char
		bool	isAtEnd() const;

		void	error(const std::string& msg) const;

	public:
		Lexer(const std::string &input);
		~Lexer();

		void	lexConf();		// loop nextToken() on _input
		
		// test functions
		void	dumpTokens();
		const std::string tokenTypeToString(tokenType type);	
};


#endif