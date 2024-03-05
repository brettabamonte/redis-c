# Define compiler and flags
CXX=g++
CXXFLAGS=-Wall -Wextra -O2

# Define source files and object files
SERVER_SRCS=src/server.cpp src/hashtable.cpp
SERVER_OBJS=$(SERVER_SRCS:.cpp=.o)
CLIENT_SRCS=src/client.cpp
CLIENT_OBJS=$(CLIENT_SRCS:.cpp=.o)

# Rule for building the server
server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o server $(SERVER_OBJS)

# Rule for building the client
client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o client $(CLIENT_OBJS)

# Generic rule for converting .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f src/*.o server
	rm -f src/*.o client