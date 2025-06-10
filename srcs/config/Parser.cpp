/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/09 16:48:56 by baptistevie       #+#    #+#             */
/*   Updated: 2025/06/10 16:52:02 by baptistevie      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/config/Parser.hpp"

// ParseError
	// constructor
ParseError::ParseError(const std::string& msg, int line, int col) : std::runtime_error(msg), _line(line), _col(col)
{ }

	// getters
int ParseError::getLine() const
{ return (_line); }

int ParseError::getColumn() const
{ return (_col); }


// Parser
	// constructor
Parser::Parser(const std::vector<token>& tokens) : _tokens(tokens), _current(0)
{ }

Parser::~Parser()
{ }

	// token management
token	Parser::peek(int offset = 0) const
{
	if (offset < 0)
		throw (std::invalid_argument("peek offset must be non-negative"));
	return (_current + offset >= _tokens.size() ? token(T_EOF, "", -1, -1) :  _tokens[_current + offset]);
}
		
token	Parser::consume()
{
	if (!isAtEnd())
		_current++;
	return (_tokens[_current - 1]);
}

bool	Parser::isAtEnd() const
{ return (_current <= _tokens.size() || checkCurrentType(T_EOF)); }
		
bool	Parser::checkCurrentType(tokenType type) const
{ return (peek().type == type); }

token Parser::expectToken(tokenType type, const std::string& context) {
	if (!checkCurrentType(type)) {
		std::ostringstream oss;
		oss << "Expected token type " << tokenTypeToString(type) << " in " << context 
			<< ", but got " << peek().value;
		error(oss.str());
	}
	return consume();
}
	// parsing methods
std::vector<ASTnode*> Parser::parse()
{
	try {
		return (parseConfig());
	} catch (const ParseError& e) {
		std::cerr	<< "Parse Error at line " << e.getLine() << ", col " << e.getColumn() 
					<< ": " << e.what() << std::endl;
		throw;
	}
}

std::vector<ASTnode *>	Parser::parseConfig()
{
	std::vector<ASTnode *>	astNodes;

	while (!isAtEnd()){
		token	current = peek();

		if (checkCurrentType(T_SERVER)) {
			astNodes.push_back(parseServerBlock());
		} else {
			std::cerr	<< "Warning: unexpected token '" << current.value
						<< "' at line " << current.line << ", col " << current.column
						<< std::endl;
			consume();
		}
	}
	return (astNodes);
}
		
BlockNode *	Parser::parseServerBlock()
{
	token	serverToken = expectToken(T_SERVER, "server block");
	BlockNode* serverBlock = new BlockNode();

    serverBlock->name = "server";
    serverBlock->line = serverToken.line;
    
    expectToken(T_LBRACE, "server block opening");
    
	// loop through the scope
    while (!checkCurrentType(T_RBRACE) && !isAtEnd()) {
		token current = peek();
		
		if (checkCurrentType(T_LOCATION)) {
			serverBlock->children.push_back(parseLocationBlock());
		} else if (checkCurrentType(T_LISTEN) ||checkCurrentType(T_SERVER_NAME) ||
					checkCurrentType(T_ERROR_PAGE) || checkCurrentType(T_CLIENT_MAX_BODY) ||
					checkCurrentType(T_INDEX) || checkCurrentType(T_ERROR_LOG)) {
			serverBlock->children.push_back(parseDirective());
		} else {
			std::cerr	<< "Warning: Unexpected directive '" << current.value 
                    	<< "' in server context at line " << current.line << ", col " << current.column
						<< std::endl;
            consume();
		}
	}

	expectToken(T_RBRACE, "server block closing");
	return (serverBlock);
}

BlockNode *	Parser::parseLocationBlock()
{
	token		locationToken = expectToken(T_LOCATION, "location block");
	BlockNode *	locationBlock = new BlockNode();
	
	locationBlock->name = "location";
	locationBlock->line = locationToken.line;

		// parse modifier (optionnal)
	std::string	modifier = parseModifier();
    if (!modifier.empty())
        locationBlock->args.push_back(modifier);
	
		// get path
	token	pathToken = peek();
	if (checkCurrentType(T_IDENTIFIER) || checkCurrentType(T_STRING)) {
		locationBlock->args.push_back(pathToken.value);
		consume();
	} else {
		unexpectedToken("locationPath");
	}

	expectToken(T_LBRACE, "location block opening");;

		// loop through the scope
	while (!isAtEnd() && !checkCurrentType(T_RBRACE)) {
		token	current = peek();

		if (checkCurrentType(T_ALLOWED_METHODS) || checkCurrentType(T_ROOT) || checkCurrentType(T_INDEX)
				|| checkCurrentType(T_AUTOINDEX) || checkCurrentType(T_UPLOAD_ENABLED) || checkCurrentType(T_UPLOAD_STORE)
				|| checkCurrentType(T_CGI_EXTENSION) || checkCurrentType(T_CGI_PATH) || checkCurrentType(T_RETURN)) {
			locationBlock->children.push_back(parseDirective());
		} else {
			std::cerr	<< "Warning: Unexpected directive '" << current.value 
                    	<< "' in location context at line " << current.line 
						<< ", col " << current.column
						<< std::endl;
            consume();
		}
	}
	
	expectToken(T_RBRACE, "location block closing");
	return (locationBlock);
}
		
DirectiveNode *	Parser::parseDirective()
{
    token	directiveToken = peek();
    
    DirectiveNode* directive = new DirectiveNode();
    directive->name = directiveToken.value;
    directive->line = directiveToken.line;
    
    consume(); // consume directive name
    
    directive->args = parseArgs();
    
    expectToken(T_SEMICOLON, "directive ending");
    
    return (directive);
}

std::vector<std::string>	Parser::parseArgs()
{
	std::vector<std::string>	args;

	while (!isAtEnd() && !checkCurrentType(T_SEMICOLON) 
			&& (checkCurrentType(T_STRING) || checkCurrentType(T_NUMBER)
				|| checkCurrentType(T_IDENTIFIER))) {
		args.push_back(peek().value);
		consume();
	}
	return (args);
}
		
std::string	Parser::parseModifier()
{ 
	token	current = peek();

	if (isModifier(current.type)) {
		consume();
		return (current.value);
	}
	return ("");
}

bool	Parser::isValidDirective(const std::string& name, const std::string& context) const
{
	if (context == "server") {
        return (name == "listen" || name == "server_name" || name == "error_page" ||
                name == "client_max_body_size" || name == "index" || name == "error_log");
    }

	if (context == "location") {
        return (name == "allowed_methods" || name == "root" || name == "index" ||
                name == "autoindex" || name == "upload_enabled" || name == "upload_store" ||
                name == "cgi_extension" || name == "cgi_path" || name == "return");
    }

    return (false);
}

bool	Parser::isModifier(tokenType type) const
{ return (type == T_EQ || type == T_TILDE || type == T_TILDE_STAR || type == T_CARET_TILDE); }

	// error management
void	Parser::error(const std::string& msg) const
{
	token	current = peek();
	throw (ParseError(msg, current.line, current.column));
}
		
void	Parser::unexpectedToken(const std::string& expected) const
{
	std::ostringstream	oss;
	
	oss	<< "Expected : '" << expected << "but got '" << peek().value << "'" ;
	error(oss.str());
}

	// cleanup
void	Parser::cleanupAST(std::vector<ASTnode*>& nodes)
{
	for (std::vector<ASTnode*>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        BlockNode*	block = dynamic_cast<BlockNode*>(*it);

        if (block)
            cleanupAST(block->children);
        delete *it;
    }
    nodes.clear();
}