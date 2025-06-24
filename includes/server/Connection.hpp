#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include "Request.hpp"
# include "Response.hpp"
# include "divers.hpp"
# include "Socket.hpp"
# include "Config.hpp"

class Connection : public Socket {
	private:
                std::vector<std::pair<Request*, Response*> >	requestResponseList;
                void        parse_header();
                void        parse_body();

	public:

		Connection();
		virtual ~Connection();

                void            handleRequest(char buf[BUFF_SIZE]);
                std::string     getRawResponse(void);
};

#endif