IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj
LDIR =../lib

#LIBS=-lm

_DEPS = daemon.h signals.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = TinyFile.o daemon.o signals.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

TinyFile: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lrt

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~