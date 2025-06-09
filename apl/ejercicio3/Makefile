CC=gcc
CFLAGS=-Wall -Wextra -g
BIN_DIR=bin

.PHONY: all cliente servidor clean

all: $(BIN_DIR)/servidor $(BIN_DIR)/cliente

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/servidor: servidor.c utils.c utils.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ servidor.c utils.c

$(BIN_DIR)/cliente: cliente.c utils.c utils.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ cliente.c utils.c


servidor: $(BIN_DIR)/servidor

cliente: $(BIN_DIR)/cliente

clean:
	rm -f $(BIN_DIR)/* /tmp/cola_impresion /tmp/FIFO_* /tmp/impresiones.log
	rmdir $(BIN_DIR) 2>/dev/null || true

