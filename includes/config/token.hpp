/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   token.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/27 15:55:14 by baptistevie       #+#    #+#             */
/*   Updated: 2025/05/28 15:58:59 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TOKEN_HPP
# define TOKEN_HPP

# include <string>

enum TokenType {
    T_IDENT,
    T_STRING,
    T_NUMBER,
    T_LBRACE,
    T_RBRACE,
    T_SEMICOLON,
    T_EQUALS,
    T_MODIFIER,
    T_COMMENT
};

struct Token {
    TokenType   type;
    std::string text;
    int         line, column;
};

#endif