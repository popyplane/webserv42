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
	$(HTTPDIR)/HttpRequestParser.cpp \
	$(HTTPDIR)/RequestDispatcher.cpp \
	$(HTTPDIR)/HttpResponse.cpp \
	$(HTTPDIR)/HttpRequestHandler.cpp

# Test source files
LEXER_TEST_SRCS = $(CONFIGDIR)/lexerTest.cpp
PARSER_TEST_SRCS = $(CONFIGDIR)/parserTest.cpp
CONFIG_LOADER_TEST_SRCS = $(CONFIGDIR)/configLoaderTest.cpp
HTTP_PARSER_TEST_SRCS = $(HTTPDIR)/httpParserTest.cpp
DISPATCHER_TEST_SRCS = $(HTTPDIR)/requestDispatcherTest.cpp
POST_DELETE_TEST_SRCS = $(HTTPDIR)/postDeleteTest.cpp # NEW: Source file for POST/DELETE tests

# Object files (using patsubst for consistency)
COMMON_CONFIG_OBJS = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(COMMON_CONFIG_SRCS))
UTILS_OBJS = $(patsubst $(UTILSDIR)/%.cpp,$(UTILSDIR)/%.o,$(UTILS_SRCS))
HTTP_OBJS = $(patsubst $(HTTPDIR)/%.cpp,$(HTTPDIR)/%.o,$(HTTP_SRCS))

LEXER_TEST_OBJ = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(LEXER_TEST_SRCS))
PARSER_TEST_OBJ = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(PARSER_TEST_SRCS))
CONFIG_LOADER_TEST_OBJ = $(patsubst $(CONFIGDIR)/%.cpp,$(CONFIGDIR)/%.o,$(CONFIG_LOADER_TEST_SRCS))
HTTP_PARSER_TEST_OBJ = $(patsubst $(HTTPDIR)/%.cpp,$(HTTPDIR)/%.o,$(HTTP_PARSER_TEST_SRCS))
DISPATCHER_TEST_OBJ = $(patsubst $(HTTPDIR)/%.cpp,$(HTTPDIR)/%.o,$(DISPATCHER_TEST_SRCS))
POST_DELETE_TEST_OBJ = $(patsubst $(HTTPDIR)/%.cpp,$(HTTPDIR)/%.o,$(POST_DELETE_TEST_SRCS)) # NEW

# Executables
LEXER_TEST_EXE = lexer_test
PARSER_TEST_EXE = parser_test
CONFIG_LOADER_TEST_EXE = config_loader_test
HTTP_PARSER_TEST_EXE = http_parser_test
DISPATCHER_TEST_EXE = dispatcher_test
POST_DELETE_TEST_EXE = post_delete_test # NEW: Executable for POST/DELETE tests

.PHONY: all clean fclean test_lexer test_parser test_config_loader test_http_parser \
		test_dispatcher test_post_delete run_tests run_lexer run_parser run_config_loader_test \
		run_http_parser_test run_dispatcher_test run_post_delete_test debug help \
		prep_post_delete_test_env

# Build all tests
all: test_lexer test_parser test_config_loader test_http_parser test_dispatcher test_post_delete # UPDATED

# Lexer test
test_lexer: $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(LEXER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(LEXER_TEST_EXE) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(LEXER_TEST_OBJ)

# Parser test
test_parser: $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(PARSER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(PARSER_TEST_EXE) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(PARSER_TEST_OBJ)

# Config Loader test
test_config_loader: $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(CONFIG_LOADER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(CONFIG_LOADER_TEST_EXE) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(CONFIG_LOADER_TEST_OBJ)

# HTTP Parser test
test_http_parser: $(HTTP_OBJS) $(UTILS_OBJS) $(HTTP_PARSER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(HTTP_PARSER_TEST_EXE) $(HTTP_OBJS) $(UTILS_OBJS) $(HTTP_PARSER_TEST_OBJ)

# Request Dispatcher test
test_dispatcher: $(HTTP_OBJS) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(DISPATCHER_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(DISPATCHER_TEST_EXE) $(HTTP_OBJS) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(DISPATCHER_TEST_OBJ)

# NEW: POST/DELETE test
test_post_delete: $(HTTP_OBJS) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(POST_DELETE_TEST_OBJ) # NEW RULE
	$(CXX) $(CXXFLAGS) -o $(POST_DELETE_TEST_EXE) $(HTTP_OBJS) $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(POST_DELETE_TEST_OBJ)

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

run_dispatcher_test: test_dispatcher
	./$(DISPATCHER_TEST_EXE)

# NEW: Run POST/DELETE test
run_post_delete_test: prep_post_delete_test_env test_post_delete
	@echo "Running POST/DELETE tests..."
	./$(POST_DELETE_TEST_EXE)
	@echo "--- POST/DELETE Test Cleanup Instructions ---"
	@echo "IMPORTANT: After testing, reset permissions on /Users/baptistevieilhescaze/dev/webserv42/www/uploads:"
	@echo "  chmod 777 /Users/baptistevieilhescaze/dev/webserv42/www/uploads"
	@echo "  rm -f /Users/baptistevieilhescaze/dev/webserv42/www/uploads/file_to_delete.txt"
	@echo "  rm -f /Users/baptistevieilhescaze/dev/webserv42/www/uploads/no_perms_delete.txt"
	@echo "  Check /Users/baptistevieilhescaze/dev/webserv42/www/uploads/ for any uploaded files (e.g., those with timestamps)."
	@echo "  These files are generated by successful POST requests and should be manually removed if desired."


run_tests: run_lexer run_parser run_config_loader_test run_http_parser_test run_dispatcher_test run_post_delete_test # UPDATED

# NEW: Target for pre-test environment setup
prep_post_delete_test_env:
	@echo "--- Preparing environment for POST/DELETE tests ---"
	@echo "1. Ensuring /Users/baptistevieilhescaze/dev/webserv42/www/uploads exists and is writable."
	mkdir -p /Users/baptistevieilhescaze/dev/webserv42/www/uploads
	chmod 777 /Users/baptistevieilhescaze/dev/webserv42/www/uploads
	@echo "2. Setting /Users/baptistevieilhescaze/dev/webserv42/www/html/protected_file.txt to read-only for TC4 of dispatcher_test."
	chmod 000 /Users/baptistevieilhescaze/dev/webserv42/www/html/protected_file.txt
	@echo "3. Cleaning up any leftover test files in /Users/baptistevieilhescaze/dev/webserv42/www/uploads."
	-rm -f /Users/baptistevieilhescaze/dev/webserv42/www/uploads/file_to_delete.txt
	-rm -f /Users/baptistevieilhescaze/dev/webserv42/www/uploads/no_perms_delete.txt
	@echo "Environment preparation complete. Proceeding with tests."


# Clean up
clean:
	rm -f $(COMMON_CONFIG_OBJS) $(UTILS_OBJS) $(HTTP_OBJS) \
		  $(LEXER_TEST_OBJ) $(PARSER_TEST_OBJ) $(CONFIG_LOADER_TEST_OBJ) $(HTTP_PARSER_TEST_OBJ) \
		  $(DISPATCHER_TEST_OBJ) $(POST_DELETE_TEST_OBJ) # UPDATED
	rm -f test_*.conf

fclean: clean
	rm -f $(LEXER_TEST_EXE) $(PARSER_TEST_EXE) $(CONFIG_LOADER_TEST_EXE) $(HTTP_PARSER_TEST_EXE) \
		  $(DISPATCHER_TEST_EXE) $(POST_DELETE_TEST_EXE) # UPDATED
	@echo "--- Final fclean cleanup instructions ---"
	@echo "Don't forget to manually clean up uploaded files:"
	@echo "  rm -rf /Users/baptistevieilhescaze/dev/webserv42/www/uploads/*"
	@echo "And ensure permissions are reset for next run:"
	@echo "  chmod 777 /Users/baptistevieilhescaze/dev/webserv42/www/html/protected_file.txt"


# Debug version with extra flags
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Help
help:
	@echo "Available targets:"
	@echo "  all                 - Build all tests (lexer, parser, config_loader, http_parser, dispatcher, post_delete)" # UPDATED
	@echo "  test_lexer          - Build lexer test only"
	@echo "  test_parser         - Build parser test only"
	@echo "  test_config_loader  - Build config loader test only"
	@echo "  test_http_parser    - Build HTTP parser test only"
	@echo "  test_dispatcher     - Build Request Dispatcher test only"
	@echo "  test_post_delete    - Build POST/DELETE test only" # NEW
	@echo "  run_lexer           - Run lexer test"
	@echo "  run_parser          - Run parser test"
	@echo "  run_config_loader_test - Run config loader test"
	@echo "  run_http_parser_test - Run HTTP parser test"
	@echo "  run_dispatcher_test - Run Request Dispatcher test"
	@echo "  run_post_delete_test - Run POST/DELETE test and prepare/cleanup environment" # NEW
	@echo "  run_tests           - Run all tests including POST/DELETE test" # UPDATED
	@echo "  prep_post_delete_test_env - Prepare directories and permissions for POST/DELETE tests" # NEW
	@echo "  debug               - Build with debug flags"
	@echo "  clean               - Clean all generated object files"
	@echo "  fclean              - Clean all generated files and executables"
	@echo "  help                - Show this help"
