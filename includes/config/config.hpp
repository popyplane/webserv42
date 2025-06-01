/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/27 17:01:52 by baptistevie       #+#    #+#             */
/*   Updated: 2025/05/28 15:58:20 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string>
#include <vector>

class Directive {
	public :
		std::string					name;
		std::vector<std::string>	values;
		bool						inherited;
};

class Context {
	public :
		virtual ~Context() {}
};

class LocationContext : public Context {
	public :
		std::string				matchType;
		std::string				pattern;
		std::vector<Directive>	directives;
};

class ServerContext : public Context {
	public:
		std::vector<Directive>			directives;  
		std::vector<LocationContext>	locations;
};

class HttpContext : public Context {
	public:
		std::vector<Directive>		directives;
		std::vector<ServerContext>	servers;
};

class MainConfig {
	public:
		std::vector<Directive>	mainDirectives;
		HttpContext				http;
};

#endif