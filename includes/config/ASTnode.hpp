/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ASTnode.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/27 15:57:55 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/21 10:25:47 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ASTNODE_HPP
# define ASTNODE_HPP

# include <string>
# include <vector>

class ASTnode {
	public :
		virtual ~ASTnode() {}
		ASTnode() : line(0), column(0) {}

		int	line, column;
};

class DirectiveNode : public ASTnode {
	public :
		std::string					name;
		std::vector<std::string>	args;

		DirectiveNode() : ASTnode() {}
};

class BlockNode : public ASTnode {
	public :
		std::string					name;
		std::vector<std::string>	args;
		std::vector<ASTnode*>		children;

		BlockNode() : ASTnode() {}
};

#endif