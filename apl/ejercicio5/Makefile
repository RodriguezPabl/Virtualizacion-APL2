# Makefile para compilar el servidor y cliente del juego Ahorcado

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDFLAGS = -pthread -lrt

# Archivos fuente
SERVER_SRC = servidor.cpp
CLIENT_SRC = cliente.cpp

# Binarios generados
SERVER_BIN = servidor
CLIENT_BIN = cliente

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)

.PHONY: all clean
