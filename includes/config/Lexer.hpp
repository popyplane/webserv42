/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/28 15:56:14 by baptistevie       #+#    #+#             */
/*   Updated: 2025/05/28 16:13:54 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_HPP
# define LEXER_HPP

# include <string>
# include "token.hpp"

class Lexer
{
	private:
		const std::string&	_input;
		size_t				_pos;
		int					_line;
		int					_column;

		void	skipWhitespaceAndComments();
    	Token	readIdentifier();
    	Token	readNumber();
    	Token	readQuotedString();
    	Token	readSymbol();
    	char	peek() const;
    	char	get();        // consume and return current char
    	bool	isAtEnd() const;

		void	error(const std::string& msg) const;

	public:
		Lexer(/* args */);
		~Lexer();
};


#endif