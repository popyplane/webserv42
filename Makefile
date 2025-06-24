# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+#+#+   +#+            #
#    Created: 2025/06/21 17:58:55 by baptistevie       #+#    #+#              #
#    Updated: 2025/06/24 16:00:00 by webserv_team     ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Makefile for testing the config parser and loader and HTTP parser

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./includes
SRCDIR = srcs
CONFIGDIR = $(SRCDIR)/config
HTTPDIR = $(SRCDIR)/http
UTILSDIR = $(SRCDIR)/utils

# Source files
# Common config component sources (StringUtils is handled by UTILS_SRCS)
COMMON_CONFIG_SRCS = \
	$(CONFIGDIR)/token.cpp \
	$(CONFIGDIR)/Lexer.cpp \
	$(CONFIGDIR)/Parser.cpp \
	$(CONFIGDIR)/ConfigLoader.cpp \
	$(CONFIGDIR)/ConfigPrinter.cpp

# Utility source files (StringUtils is used by both config and http modules)
UTILS_SRCS = \
	$(UTILSDIR)/StringUtils.cpp

# HTTP component sources
HTTP_SRCS = \
	$(HTTPDIR)/HttpRequest.cpp \
	$(HTTPDIR)/HttpRequestParser.cpp

# Test source files
LEXER_TEST_SRCS = $(CONFIGDIR)/lexerTest.cpp
PARSER_TEST_SRCS = $(CONFIGDIR)/parserTest.cpp
CONFIG_LOADER_TEST_SRCS = $(CONFIGDIR)/configLoaderTest.cpp
HTTP_PARSER_TEST_SRCS = $(HTTPDIR)/httpParserTest.cpp

# Object files (using patsubst is fine for generating the list of .o files)
COMMON_CONFIG_OBJS = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(COMMON_CONFIG_SRCS))
# Corrected expansion for UTILS_OBJS to avoid directory issues in rm
UTILS_OBJS = $(patsubst $(UTILSDIR)/%.cpp,$(UTILSDIR)/%.o,$(UTILS_SRCS))
HTTP_OBJS = $(patsubst $(HTTPDIR)/%.cpp,$(HTTPDIR)/%.o,$(HTTP_SRCS))

LEXER_TEST_OBJ = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(LEXER_TEST_SRCS))
PARSER_TEST_OBJ = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(PARSER_TEST_SRCS))
CONFIG_LOADER_TEST_OBJ = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(CONFIG_LOADER_TEST_SRCS))
HTTP_PARSER_TEST_OBJ = $(patsubst $(HTTPDIR)/%.cpp,$(HTTPDIR)/%.o,$(HTTP_PARSER_TEST_SRCS))

# Executables
LEXER_TEST_EXE = lexer_test
PARSER_TEST_EXE = parser_test
CONFIG_LOADER_TEST_EXE = config_loader_test
HTTP_PARSER_TEST_EXE = http_parser_test

.PHONY: all clean fclean test_lexer test_parser test_config_loader test_http_parser run_tests \
		run_lexer run_parser run_config_loader_test run_http_parser_test debug help

# Build all tests (includes HTTP parser test now)
all: test_lexer test_parser test_config_loader test_http_parser

# Lexer test
test_lexer: $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(LEXER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(LEXER_TEST_EXE) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(LEXER_TEST_OBJ)

# Parser test
test_parser: $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(PARSER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(PARSER_TEST_EXE) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(PARSER_TEST_OBJ)

# Config Loader test
test_config_loader: $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(CONFIG_LOADER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(CONFIG_LOADER_TEST_EXE) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(CONFIG_LOADER_TEST_OBJ)

# HTTP Parser test (NEW TARGET)
test_http_parser: $(HTTP_OBJS) $(UTILS_OBJS) $(HTTP_PARSER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(HTTP_PARSER_TEST_EXE) $(HTTP_OBJS) $(UTILS_OBJS) $(HTTP_PARSER_TEST_OBJ)

# Compile individual source files using specific pattern rules
$(CONFIGDIR)/%.o: $(CONFIGDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(HTTPDIR)/%.o: $(HTTPDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(UTILSDIR)/%.o: $(UTILSDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run tests
run_lexer: test_lexer
	./$(LEXER_TEST_EXE)

run_parser: test_parser
	./$(PARSER_TEST_EXE)

run_config_loader_test: test_config_loader
	./$(CONFIG_LOADER_TEST_EXE)

run_http_parser_test: test_http_parser 
	./$(HTTP_PARSER_TEST_EXE)

run_tests: run_lexer run_parser run_config_loader_test run_http_parser_test

# Clean up
clean:
	rm -f $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(HTTP_OBJS) \
		  $(LEXER_TEST_OBJ) $(PARSER_TEST_OBJ) $(CONFIG_LOADER_TEST_OBJ) $(HTTP_PARSER_TEST_OBJ)
	rm -f test_*.conf

fclean: clean
	rm -f $(LEXER_TEST_EXE) $(PARSER_TEST_EXE) $(CONFIG_LOADER_TEST_EXE) $(HTTP_PARSER_TEST_EXE)

# Debug version with extra flags
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Help
help:
	@echo "Available targets:"
	@echo "  all                 - Build all tests (lexer, parser, config_loader, http_parser)"
	@echo "  test_lexer          - Build lexer test only"
	@echo "  test_parser         - Build parser test only"
	@echo "  test_config_loader  - Build config loader test only"
	@echo "  test_http_parser    - Build HTTP parser test only"
	@echo "  run_lexer           - Run lexer test"
	@echo "  run_parser          - Run parser test"
	@echo "  run_config_loader_test - Run config loader test"
	@echo "  run_http_parser_test - Run HTTP parser test"
	@echo "  run_tests           - Run all tests"
	@echo "  debug               - Build with debug flags"
	@echo "  clean               - Clean all generated files"
	@echo "  fclean              - Clean all generated files and executables"
	@echo "  help                - Show this help"