#include "Server.hpp"

Server::Server(Config* conf) {
    this->_fd_n = 0;
	this->_fd_max = MAXEVENTS;
	_pfds = new pollfd[_fd_max];

    _config = conf;
    // Initialize the listening ports from the config
    int i = 0;
    for (std::vector<ServerBlock*>::iterator it = _config->_server_blocks.begin(); it != _config->_server_blocks.end(); ++it) {
        std::cout << "- launching a server on port " << (*it)->getListeningPort();
        std::cout << " at pfds[" << i << "]" << std::endl;
        // Add a listening socket to the list
        // TODO: memory clean
        Socket* listenSock = new Socket;
        listenSock->setPortFD((*it)->getListeningPort());
        // Add relevant serverblock to listening socket!
        listenSock->setServerBlock(*it);
        _listenSockets.push_back(listenSock);
      	_listenSockets.back()->initListenSocket((*it)->getListeningPort().c_str());
    	_pfds[i].fd = _listenSockets.back()->getSocketFD();
	    _pfds[i].events = POLLIN | POLLOUT;
	    _fd_n++;
        i++;
        std::cout << std::endl;
    }// a faire a chat gpt
}

Server::~Server() {
    delete[] _pfds;
    std::map<int, Connection*>::iterator it;
    for (it = _connections.begin(); it != _connections.end(); ++it)
    {
        //if (it->second)
        delete it->second;
    }
}

void Server::run(void){
    int nbPollOpen;
    while (1)
    {
        if ((nbPollOpen = poll(_pfds, _fd_n, -1)) < 0){
            throw("Poll error...\n");
        }
        for(int i = 0; i < _fd_n; i++)
        {
            if (_pfds[i].revents & POLLHUP)
                dropConnection(i);
            else if (_pfds[i].revents & POLLIN)
            {//si trouve un socket d'ecoute, ca veux dire que c'est un debut d'ecoute
                Socket* listenSock = retrieveListeningSocket(_pfds[i].fd);
                if (listenSock != NULL)
                {
					handleNewConnection(listenSock);
                    break ;
                }//else//si pas de socket trouve, ca veux dire que c'est en cours sans doute
				readFromExistingConnection(i);
            }//simplifiable fort
            else if (_pfds[i].revents & POLLOUT)
                respondToExistingConnection(i);
        }
    }
}

void	Server::dropConnection(int i) {
    _connections[_pfds[i].fd]->closeSocket();
    std::map<int, Connection*>::iterator it = _connections.find(_pfds[i].fd);
    if (it != _connections.end())//check pas la fin de la map
    {
        delete it->second;//value
        _connections.erase(it);
    }
    if (i != _fd_n - 1)//remplace la connexion perdue par la plus recente
        _pfds[i] = _pfds[_fd_n - 1];
	_fd_n--;//diminue nb fd
    std::cout << "Socket Close Succelly" << std::endl;
}

Socket*    Server::retrieveListeningSocket(int fd) {
    for (std::vector<Socket*>::iterator it = _listenSockets.begin(); it != _listenSockets.end(); it++){
        if (fd == (*it)->getSocketFD())
            return (*it) ;
    }
    return (NULL);//compare chaque fd au socket actuel, et renvoie le lien si il trouve 
}

void	Server::handleNewConnection(Socket* listenSock) {// a retrazvailler
	socklen_t				addrlen;
	struct sockaddr_storage	remote_addr;
	char					remoteIP[INET_ADDRSTRLEN];
    Connection*             new_connection = new Connection();

	addrlen = sizeof(remote_addr);
    try {
	    new_connection->setSocketFD(accept(listenSock->getSocketFD(), (struct sockaddr *)&remote_addr, &addrlen));// Set the socket for the connection
        new_connection->setServerBlock(listenSock->getServerBlock());// Add server block config to this connection
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    addConnection(new_connection->getSocketFD(), new_connection);	// add connection to list of existing connections in pfds
	std::cout << "New connexion " << inet_ntop(remote_addr.ss_family, get_in_addr((struct sockaddr*)&remote_addr), remoteIP, INET_ADDRSTRLEN);
	std::cout << " on socket " << new_connection->getSocketFD();
    std::cout << " over port " << new_connection->getServerBlock()->getListeningPort() << std::endl;
}

void	Server::readFromExistingConnection(int i) {// A modif en try catch
	int	    n;
	char    str[BUFF_SIZE];

	memset(str, 0, BUFF_SIZE);
    n = recv(_pfds[i].fd, str, sizeof(str), 0);
	if (n <= 0)
	{
		if (n == 0)// pas d'erreur : co fermee par le client
			std::cout << "Socket closed : " << _connections[_pfds[i].fd]->getSocketFD() << std::endl;
		else if (n < 0)//erreur
            dropConnection(i);
		dropConnection(i);// repetition pas forcement nessessaire
	}
	else
	{
        _connections[_pfds[i].fd]->handleRequest(str);
        memset(str, 0, BUFF_SIZE);
	}
}

void    Server::respondToExistingConnection(int i) {
    std::string     str;

    try {
        str = _connections[_pfds[i].fd]->getRawstr();
    } catch (std::exception& e) {
        std::cerr << "error with repsonse to existing socket..." << std::endl;
    }
    if (str.empty())
        return ;

    char* buffer = new char[str.size()];//add protzection !!!
    std::memcpy(buffer, str.c_str(), str.size());
    int bytes_sent = send(_pfds[i].fd, buffer, str.size(), 0);
    if (str.find("302 Found") != std::string::npos
        || str.find("404 Not Found") != std::string::npos
        || str.find("204 No Content") != std::string::npos
        || str.find("413 Content Too Large") != std::string::npos
        || str.find("401 Unauthorized") != std::string::npos
		)
    {
        std::cout << "End connect" << std::endl;
        dropConnection(i);
    }// permet de ne traiter que les reponses attendues
    delete[] buffer;
    if (bytes_sent <= 0)
    {
        std::cerr << "error with send for respond..." << std::endl;
        dropConnection(i);
    }//try catch possible pour amelio
}

void	Server::addConnection(int newfd, Connection* new_conn) {
	if (_fd_n == _fd_max)
	{
		_fd_max *= 2;// pour reallouer exponotiellement et s'adapter au besoin
		struct pollfd *pfds_new = new pollfd[_fd_max];
		for (int i = 0; i < _fd_n; i++)
		{
			pfds_new[i].fd = _pfds[i].fd;
			pfds_new[i].events = _pfds[i].events;
			pfds_new[i].revents = _pfds[i].revents;
		}
		delete[] _pfds;
		_pfds = pfds_new;
	}
	_pfds[_fd_n].fd = newfd;
	_pfds[_fd_n].events = POLLIN | POLLOUT;
	_fd_n++;

    // Add a connection to the list of connections to the server
    _connections.insert(std::make_pair(newfd, new_conn));// check fct fct : map?
}

void* Server::get_in_addr(struct sockaddr *sa) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
}