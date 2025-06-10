/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mainTest.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/03 16:16:34 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/03 17:00:47 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/config/Lexer.hpp"

int	main()
{
	std::string	tmp;
	readFile("configs/basic.conf", tmp);
	Lexer lexer(tmp);
	lexer.lexConf();
	lexer.dumpTokens();
}