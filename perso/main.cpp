#include "webserv.hpp"

int main(int argc, char **argv){
    if(argc < 1 || argc > 2)
    {
        //std::cerr? cout? ou autre pour etre au bon endroit en sortie -> wrong arg
        std::cerr << "Please use correct format : ./webserv or ./webserv [config file]" << std::endl;
        return (1);
    }
    Config config;//declaration des class
    try{
        if (argc == 1)
            std::string fConf("default.conf");
        else
            std::string fConf(argv[1]);
        config.checkConfig(fConf);//definir string puis allouer en une fois
    }
    catch (std::exception &e){
        std::cerr << "Error in checkconfig..." << std::endl;
        return (1);
    }
    config.printConfig();//A voir si utile
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