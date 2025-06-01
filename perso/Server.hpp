#ifndef SERVER_HPP
# define SERVER_HPP

# include "Config.hpp"
class Server
{
private:
    Server();//ne doit pas etre lancé sans parametre
public:
    //initialisation + forme coplienne
    Server(Config *conf);
    Server(const Server &cpy);
    Server &operator=(const Server &src);
    ~Server();



    //methodes principales
    void run();
};



#endif