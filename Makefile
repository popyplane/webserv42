# Makefile for testing the config parser

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./includes
SRCDIR = srcs
CONFIGDIR = $(SRCDIR)/config
TESTDIR = test

# Source files
LEXER_SRCS = $(CONFIGDIR)/Lexer.cpp $(CONFIGDIR)/token.cpp
PARSER_SRCS = $(CONFIGDIR)/Parser.cpp
TEST_SRCS = $(CONFIGDIR)/parserTest.cpp

# Object files
LEXER_OBJS = $(LEXER_SRCS:.cpp=.o)
PARSER_OBJS = $(PARSER_SRCS:.cpp=.o)
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

# Targets
LEXER_TEST = lexer_test
PARSER_TEST = parser_test

.PHONY: all clean test_lexer test_parser

all: test_lexer test_parser

# Lexer test (your existing test)
test_lexer: $(LEXER_OBJS) $(CONFIGDIR)/lexerTest.o
	$(CXX) $(CXXFLAGS) -o $(LEXER_TEST) $^

# Parser test (comprehensive test)
test_parser: $(LEXER_OBJS) $(PARSER_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PARSER_TEST) $^

# Compile individual source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run tests
run_lexer: test_lexer
	./$(LEXER_TEST)

run_parser: test_parser
	./$(PARSER_TEST)

run_tests: run_lexer run_parser

# Clean up
clean:
	rm -f $(LEXER_OBJS) $(PARSER_OBJS) $(TEST_OBJS) $(CONFIGDIR)/lexerTest.o
	rm -f $(LEXER_TEST) $(PARSER_TEST)
	rm -f test_*.conf

# Debug version with extra flags
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Help
help:
	@echo "Available targets:"
	@echo "  all         - Build both lexer and parser tests"
	@echo "  test_lexer  - Build lexer test only"
	@echo "  test_parser - Build parser test only"
	@echo "  run_lexer   - Run lexer test"
	@echo "  run_parser  - Run parser test"
	@echo "  run_tests   - Run both tests"
	@echo "  debug       - Build with debug flags"
	@echo "  clean       - Clean all generated files"
	@echo "  help        - Show this help"