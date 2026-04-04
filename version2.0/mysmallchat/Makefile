CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Isrc

SERVER_TARGET = server
CLIENT_TARGET = client

COMMON_SRCS = src/Log.cpp src/socketutil.cpp src/epoll.cpp 

SERVER_SRCS = server.cpp \
			  src/chatserver.cpp \
			  src/clientsession.cpp \
			  src/handleclient.cpp \
			  $(COMMON_SRCS)
			  
CLIENT_SRCS = client.cpp \
			  src/chatclient.cpp \
			  src/clientstate.cpp \
			  $(COMMON_SRCS)

SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

.PHONY: all clean server client

all: $(SERVER_TARGET) $(CLIENT_TARGET)

server: $(SERVER_TARGET)

client: $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGES) -o $@ $^

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGES) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGES) -c $< -o $@

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_TARGET) $(CLIENT_TARGET)


