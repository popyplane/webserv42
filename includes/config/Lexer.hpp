/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/28 15:56:14 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/02 17:42:54 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_HPP
# define LEXER_HPP

# include <string>
# include <fstream>
# include <cctype>
# include "token.hpp"

bool	readFile(const std::string &fileName, std::string &out);

class Lexer
{
	private:
		const std::string&	_input;
		size_t				_pos;
		int					_line;
		int					_column;
		std::vector<Token>	_tokens;

		Token	nextToken();
		void	skipWhitespaceAndComments();
    	Token	tokeniseIdentifier();
    	Token	tokeniseNumber();
    	Token	tokeniseString();
    	Token	tokeniseSymbol();
		Token	tokeniseModifier();
    	char	peek() const;
    	char	get();        // consume and return current char
    	bool	isAtEnd() const;

		void	error(const std::string& msg) const;

	public:
		Lexer(/* args */);
		~Lexer();

		
};


#endif