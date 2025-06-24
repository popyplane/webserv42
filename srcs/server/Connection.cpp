#include "../../includes/server/Connection.hpp"

Connection::Connection() {
}

Connection::~Connection() {
}

void    Connection::handleRequest(char str[BUFF_SIZE]) {//A MODIF A PARTIR DE REQUEST
	int n = 0;
    while (n < BUFF_SIZE)
    {
        if (requestResponseList.size() == 0
            || requestResponseList[requestResponseList.size() - 1].first->isFullyParsed())
        {//check si pas de requete en cours de traitement, ou si end requete entierement analysee 
            Request* req = new Request;
            std::pair<Request*, Response*> my_pair = std::make_pair(req, (Response*)0);
            requestResponseList.push_back(my_pair);
	    }//associe la new requete a une reponse pas encore definie
	    Request* last_req = requestResponseList[requestResponseList.size() - 1].first;
        try {
	        n = last_req->parseRequest(str, n, getServerBlock());
            last_req->parse_status = "OK";
			last_req->setStatusCode(200);
        }
		catch (std::exception& e) {
            n = BUFF_SIZE;//fini la boucle while
			try {
				e = dynamic_cast<Request::BadRequestException &>(e);
				last_req->setStatusCode(400);
			} catch (std::bad_cast const&) {}
			try {
				e = dynamic_cast<Request::NotFoundException &>(e);
				last_req->setStatusCode(404);
			} catch (std::bad_cast const&) {}
			try {
				e = dynamic_cast<Request::HttpVersionNotSupportedException &>(e);
	            last_req->parse_status = "VersionMismatch";
				last_req->setStatusCode(505);
			} catch (std::bad_cast const&) {}
			try {
				e = dynamic_cast<Request::ContentTooLargeException &>(e);
	            last_req->parse_status = "ContentTooLarge";
				last_req->setStatusCode(413);
			} catch (std::bad_cast const&) {}
            try {
				e = dynamic_cast<Request::UnauthorizedException &>(e);
	            last_req->parse_status = "Unauthorized";
				last_req->setStatusCode(401);
			} catch (std::bad_cast const&) {}
            try {
				e = dynamic_cast<Request::RedirectException &>(e);
	            last_req->parse_status = "Redirect";
                last_req->setStatusCode(302);
			} catch (std::bad_cast const&) {}
		}
	}
}

std::string Connection::getRawResponse(void) {
	std::string str_rep;
    //iterite tte les paires requetes-reponses et agis en consequence pouir chacune
    for (std::vector<std::pair<Request*, Response*> >::iterator it = requestResponseList.begin(); it != requestResponseList.end(); ++it) {
	    Request* req = it->first;
		if (req->getStatusCode() != OK)//MACRO OK a definir
        {
	        it->second = new NotOkResponse();
            //tente de recup un chemin de page d'erreur personalisé via config
	        std::string errorPath = getServerBlock()->getErrorPath(req->getStatusCode());
	        if (!errorPath.empty())//construit en fonction de celle ci
	            it->second->constructConfigResponse(*req, errorPath);
            else//page d'erreur par defaut est construite
                it->second->constructDefaultResponseWithBody(*req, getDefaultErrorPage(req->getStatusCode()));
	        str_rep = it->second->getRawResponse();//chaine de reponse brute generee a partir de reponse
            delete it->first;
            delete it->second;
            requestResponseList.erase(it);//la paire est enlevee de la liste
        }
        else if (req->isFullyParsed()) {//with enum (+ pratique), modif par map/vector possible
            try {
                if (req->getRequestMethod() == GET)
                    it->second = new GetResponse(getServerBlock());
                if (req->getRequestMethod() == POST)
                    it->second = new PostResponse(getServerBlock());
                if (req->getRequestMethod() == DELETE)
                    it->second = new DeleteResponse(getServerBlock());
            } catch (std::exception& e) {
                throw ("Error with request\n");
            }
            it->second->constructResponse(*req);//construit avec la methode appropriee
            str_rep = it->second->getRawResponse();
            delete it->first;
            delete it->second;
            requestResponseList.erase(it);
        }
    }
    return str_rep;
}