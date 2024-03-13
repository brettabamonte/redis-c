# Define compiler and flags
CXX=g++
CXXFLAGS=-Wall -Wextra -O2

#Directories
SRCDIR = src
TESTDIR = tests
BINDIR = bin

# Define source files and object files
SERVER_SRCS=src/server.cpp src/hashtable.cpp src/utils.cpp src/zset.cpp src/avl.cpp
SERVER_OBJS=$(SERVER_SRCS:.cpp=.o)
CLIENT_SRCS=src/client.cpp src/utils.cpp
CLIENT_OBJS=$(CLIENT_SRCS:.cpp=.o)
TEST_SRCS=tests/avl-test.cpp src/avl.cpp src/utils.cpp src/hashtable.cpp
TEST_OBJS=$(TEST_SRCS:.cpp=.o)

# Rule for building the server
server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/server $(SERVER_OBJS)

# Rule for building the client
client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/client $(CLIENT_OBJS)

#Rule for building tests
tests: $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/tests $(TEST_OBJS)

# Generic rule for converting .cpp files to .o files
$(BINDIR)/%.o: $(SRCDIR)/%.cpp $(TESTDIR)/%.cpp | $(BINDIR)/.dir
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove object files and executables
clean:
	rm -rf $(BINDIR)/*
	rm -rf $(SRCDIR)/*.o