#include "../../includes/server/Socket.hpp"

Socket::Socket() {
}

Socket::~Socket() {
}

Socket::Socket(const Socket	&cpy) {
	this->_sockfd = cpy._sockfd;
	this->_sin_size = cpy._sin_size;
}

Socket& Socket::operator=(const Socket	&src) {
	this->_sockfd = src._sockfd;
	this->_sin_size = src._sin_size;
	return (*this);
}

//recup methode d'adrewsage ip
static void	*get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}//savoir quelle methode ip est utilisee

//creation de socket unitaire : prepare le point de com
void	Socket::createSocket(int ai_family, int ai_socktype, int ai_protocol) {
	int yes = 1;

	if ((_sockfd = socket(ai_family, ai_socktype, ai_protocol)) < 0)
		throw ("error with socket");
	if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
		throw ("error with socket opt");
}

//lie socket et addrIP/port : pour que serv ecoute port 8080 a ip 2.2.2.2 :
// lie le socket a ses port d'entrees des donnees (etabli le chemin d'acces au socket)
void	Socket::bindSocket(struct sockaddr* ai_addr, socklen_t ai_addrlen) {
	if (bind(_sockfd, ai_addr, ai_addrlen) < 0)
		throw ("error with socket bind");
}

//socket allumé : prete a accepter des connexion
void	Socket::listenOnSocket(void) {
	if (listen(_sockfd, BACKLOG) < 0)
		throw ("error with listen socket");
	std::cout << "listen socket : " << _sockfd << std::endl;
}

//accpeter les connexion entrantes via le socket
void	Socket::acceptConnection(int listenSock) {
	if ((_sockfd = accept(listenSock, (struct sockaddr*)&_addr, &_sin_size)) < 0)
		throw ("error with accept socket");
}

//afficher sure la console que le server a accepté une connection precise
void	Socket::printConnection(void) {
	char s[INET6_ADDRSTRLEN];

	inet_ntop(_addr.ss_family, get_in_addr((struct sockaddr *)&_addr), s, sizeof(s));
	std::cout << "server received connection from: " << s << std::endl;
}

//Fct gestion d'activation de socket
void	Socket::initListenSocket(const char* port) {
	struct addrinfo	base;
	struct addrinfo	*ai;
	struct addrinfo *p;

	memset(&base, 0, sizeof(base));
	base.ai_family = AF_INET;
    base.ai_socktype = SOCK_STREAM;
    base.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, port, &base, &ai) != 0)
		throw ("error with init socket");
	for (p = ai; p != NULL; p = p->ai_next)
	{
		try {
			createSocket(p->ai_family, p->ai_socktype, p->ai_protocol);
		} catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
			continue ;
		}
		try {
			bindSocket(p->ai_addr, p->ai_addrlen);
		} catch (std::exception& e) {
            close(_sockfd);
			std::cerr << e.what() << std::endl;
			continue ;
		}
		break ;
	}
	freeaddrinfo(ai);
	if (p == NULL)
		throw ("error with socket bind");
	listenOnSocket();
}

//fermeture propre d'un socket
void    Socket::closeSocket(void) {
    int n;

    n = close(this->getSocketFD());
    if (n < 0)
        throw ("Error with socket close");
    std::cout << "SOCKET " << this->getSocketFD() << " CLOSED" << std::endl;
}

int		Socket::getSocketFD(void) {
	return (this->_sockfd);
}

ServerConfig*    Socket::getServerBlock(void) {
    return (_server_block);
}

void    Socket::setSocketFD(int fd) {
    if (fd < 0)
        throw ("Error fd incorrect");
    this->_sockfd = fd;
}

void    Socket::setPortFD(std::string port) {
    this->_port = port;
}

void    Socket::setServerBlock(ServerConfig* sb) {
    this->_server_block = sb;
}