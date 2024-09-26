CC=gcc
CFLAGS=-I.
DEPS = daemon.h
OBJ = TinyFile.o daemon.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

TinyFile: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)