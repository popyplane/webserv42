# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: baptistevieilhescaze <baptistevieilhesc    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+#+#+   +#+            #
#    Created: 2025/06/21 17:58:55 by baptistevie       #+#    #+#              #
#    Updated: 2025/06/21 18:05:00 by webserv_team     ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Makefile for testing the config parser and loader

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./includes
SRCDIR = srcs
CONFIGDIR = $(SRCDIR)/config

# Source files
# Common config component sources
COMMON_CONFIG_SRCS = \
	$(CONFIGDIR)/token.cpp \
	$(CONFIGDIR)/Lexer.cpp \
	$(CONFIGDIR)/Parser.cpp \
	$(CONFIGDIR)/ConfigLoader.cpp \
	$(CONFIGDIR)/StringUtils.cpp \
	$(CONFIGDIR)/ConfigPrinter.cpp # New: ConfigPrinter source

# Test source files
LEXER_TEST_SRCS = $(CONFIGDIR)/lexerTest.cpp
PARSER_TEST_SRCS = $(CONFIGDIR)/parserTest.cpp
CONFIG_LOADER_TEST_SRCS = $(CONFIGDIR)/configLoaderTest.cpp # New: Config Loader Test

# Object files (explicitly define paths for objects in CONFIGDIR)
COMMON_CONFIG_OBJS = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(COMMON_CONFIG_SRCS))
LEXER_TEST_OBJ = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(LEXER_TEST_SRCS))
PARSER_TEST_OBJ = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(PARSER_TEST_SRCS))
CONFIG_LOADER_TEST_OBJ = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(CONFIG_LOADER_TEST_SRCS))

# Targets
LEXER_TEST_EXE = lexer_test
PARSER_TEST_EXE = parser_test
CONFIG_LOADER_TEST_EXE = config_loader_test # New: Executable for Config Loader Test

.PHONY: all clean fclean test_lexer test_parser test_config_loader run_tests

all: test_lexer test_parser test_config_loader

# Lexer test
test_lexer: $(COMMON_CONFIG_OBJS) $(LEXER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(LEXER_TEST_EXE) $(COMMON_CONFIG_OBJS) $(LEXER_TEST_OBJ)

# Parser test
test_parser: $(COMMON_CONFIG_OBJS) $(PARSER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(PARSER_TEST_EXE) $(COMMON_CONFIG_OBJS) $(PARSER_TEST_OBJ)

# Config Loader test (NEW TARGET)
test_config_loader: $(COMMON_CONFIG_OBJS) $(CONFIG_LOADER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(CONFIG_LOADER_TEST_EXE) $(COMMON_CONFIG_OBJS) $(CONFIG_LOADER_TEST_OBJ)

# Compile individual source files in CONFIGDIR using a specific pattern rule
$(CONFIGDIR)/%.o: $(CONFIGDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run tests
run_lexer: test_lexer
	./$(LEXER_TEST_EXE)

run_parser: test_parser
	./$(PARSER_TEST_EXE)

run_config_loader_test: test_config_loader # New: Run Config Loader Test
	./$(CONFIG_LOADER_TEST_EXE)

run_tests: run_lexer run_parser run_config_loader_test # Include new test in overall run

# Clean up
clean:
	rm -f $(COMMON_CONFIG_OBJS) $(LEXER_TEST_OBJ) $(PARSER_TEST_OBJ) $(CONFIG_LOADER_TEST_OBJ)
	rm -f test_*.conf

fclean: clean
	rm -f $(LEXER_TEST_EXE) $(PARSER_TEST_EXE) $(CONFIG_LOADER_TEST_EXE) # Include new executable


# Debug version with extra flags
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Help
help:
	@echo "Available targets:"
	@echo "  all                 - Build all tests (lexer, parser, config_loader)"
	@echo "  test_lexer          - Build lexer test only"
	@echo "  test_parser         - Build parser test only"
	@echo "  test_config_loader  - Build config loader test only"
	@echo "  run_lexer           - Run lexer test"
	@echo "  run_parser          - Run parser test"
	@echo "  run_config_loader_test - Run config loader test"
	@echo "  run_tests           - Run all tests"
	@echo "  debug               - Build with debug flags"
	@echo "  clean               - Clean all generated files"
	@echo "  fclean              - Clean all generated files and executables"
	@echo "  help                - Show this help"
