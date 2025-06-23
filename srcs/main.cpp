#include "webserv.hpp"

int main(int argc, char **argv){
    if(argc < 1 || argc > 2)
    {
        //std::cerr? cout? ou autre pour etre au bon endroit en sortie -> wrong arg
        std::cerr << "please use [./webserv] or [./webserv *.conf]" << std::endl;
        return (1);
    }
    Config config;//declaration des class
    try{
        if (argc == 2)
            std::string fConf(argv[1]);
        else
            std::string fConf("default.conf");
        config.checkConfig(fConf);//definir string puis allouer en une fois
    }
    catch (std::exception &e){
        std::cerr << "Config error occured, plese check if your file is good!" << std::endl;
        return (1);
    }
    Server server(&config);
    try {
        server.run();//boucle principale
    }
    catch (std::exception &e){
        std::cerr << "Webserv off" << std::endl;
    }
    return (0);
}
//     try{
//         //signal(SIGPIPE, f*sighandle); -> gerer si sig pipe (err, type fermture client)
//         //choix config : fournie ou default
//         //initialisation
//         //boucle -> prio ->f*run
//     }
//     catch (std::exception &e){
//         std::cerr << e.what() << std::endl;
// //        return (1); on considere que ca cree une erreur majeure?
//     }

// void    fRun(){
//     //while(1, true, etc){
//     //1.check all ok (nb conn max, etc)
//     //2.check si : new co, co dispo, lis contenu ou envoie les requetes
//     //3.check time out -> non bloquant, timeout via poll et fixe a notre jugé
//     //}
// }