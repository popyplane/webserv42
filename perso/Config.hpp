#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <iostream>

class Config{//recup toutes les infos du files de conf
    private:

    public:
    void checkConfig(std::string fConf);
    void printConfig(void);
};

#endif