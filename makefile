CC=g++
CFLAGS=-g --std=c++11 -I ./include

SERVER_TARGET = tcp_server
CLIENT_TARGET = tcp_cli
OBJS-DIR = ./build

LIBS = -lpthread #-Wl,-rpath ./lib

# MAIN_SOURCE=${wildcard ./*.cpp}
# OBJS-MAIN = ${patsubst ./%.cpp, $(OBJS-DIR)/%.o, $(MAIN_SOURCE)}

# OBJS = $(OBJS-MAIN)

VPATH = examples

all: $(SERVER_TARGET) $(CLIENT_TARGET)
$(SERVER_TARGET): tcp_server_main.cpp
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(CLIENT_TARGET): tcp_cli_main.cpp
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# $(OBJS-DIR)/%.o: %.cpp 
# 	@echo $@ $^
# 	$(CC) -o $@ -c $^ $(CFLAGS)

.PHONY: install
install:
	@echo "install" 

.PHONY: clean
clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)