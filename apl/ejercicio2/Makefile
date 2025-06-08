CC = gcc
CFLAGS = -Wall -pthread -g
OBJ = main.o buffer.o productor.o consumidor.o
BIN = ejercicio2

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)

main.o: main.c buffer.h productor.h consumidor.h

buffer.o: buffer.c buffer.h

productor.o: productor.c productor.h buffer.h

consumidor.o: consumidor.c consumidor.h buffer.h

clean:
	rm -f *.o $(BIN)
