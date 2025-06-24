#ifndef SOCKET_HPP
# define SOCKET_HPP

# include "divers.hpp"
# include "ServerStructures.hpp"

class Socket {
	private:
		int						_sockfd;
		socklen_t				_sin_size;
		struct sockaddr_storage	_addr;
        std::string             _port;
        ServerStruct*            _server_block;

	public:
		// Constructors
		Socket();
		virtual ~Socket();
		Socket(const Socket& cpy);
		Socket& operator=(const Socket& src);

		// Methods
		void	createSocket(int ai_family, int ai_socktype, int ai_protocol);
		void	bindSocket(struct sockaddr* ai_addr, socklen_t ai_addrlen);
		void	listenOnSocket(void);
		void	acceptConnection(int listenSock);
		void	printConnection(void);
		void	initListenSocket(const char* port);
        void    closeSocket(void);

		int		        getSocketFD(void);
        int             getPort(void);
        ServerStruct*    getServerBlock(void);

        void    setSocketFD(int fd);
        void    setPortFD(std::string port);
        void    setServerBlock(ServerStruct* sb);
};

#endif