#include "Server.hpp"

Server::Server(Config* conf) {}

Server::~Server() {}

Server::Server(const Server &cpy){}

Server&	Server::operator=(const Server &cpy) {}

void Server::run(void){
    int nbPollOpen;
    while (1)
    {
        //gestion poll
    }
}