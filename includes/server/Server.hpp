#ifndef SERVER_HPP
# define SERVER_HPP

# include "../webserv.hpp"
# include "Socket.hpp"

class Server
{
private://ne doit pas etre lancé sans parametre

    std::vector<Socket*>         _listenSockets;
	Socket				        _listenSocket;
	struct addrinfo		        _hints;
	struct addrinfo*	        _servinfo;
	int					        _fd_n;
	int					        _fd_max;
	struct pollfd   	    	*_pfds;
	ServerConfig*						_config;
    std::map<int, Connection*>  _connections;






public:
    Server(ServerConfig *config);
    virtual ~Server();

    //methode principale
    void run();

    //methodes associees
    void	dropConnection(int i);
    Socket*    retrieveListeningSocket(int fd);
    void	handleNewConnection(Socket* listenSock);
    void	readFromExistingConnection(int i);
    void    respondToExistingConnection(int i);
    void	addConnection(int newfd, Connection* new_conn);
    void*	get_in_addr(struct sockaddr *sa);
};



#endif