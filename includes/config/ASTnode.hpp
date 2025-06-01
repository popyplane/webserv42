/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ASTnode.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/27 15:57:55 by baptistevie       #+#    #+#             */
/*   Updated: 2025/05/28 15:57:44 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ASTNODE_HPP
# define ASTNODE_HPP

# include <string>
# include <vector>

class ASTnode {
	public :
		virtual ~ASTnode() {}
};

class DirectiveNode : public ASTnode {
	public :
		std::string					name;
		std::vector<std::string>	args;
		int							line;
};

class BlockNode : public ASTnode {
	public :
		std::string					name;
		std::vector<std::string>	args;
		std::vector<ASTnode*>		children;
		int							line;
};

#endif