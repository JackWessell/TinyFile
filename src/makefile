IDIR =./include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=../bin/obj
BINDIR=../bin
SYS_LIBS = -lrt -lpthread
_DEPS = daemon.h  utils.h mq.h clientutils.h daemonutils.h signals.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = TinyFile.o daemon.o  utils.o mq.o daemonutils.o signals.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_OBJ2 = TinyClient.o utils.o mq.o clientutils.o signals.o
OBJ2 = $(patsubst %,$(ODIR)/%,$(_OBJ2))
#compile our include functions.
$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

#compile TinyFile initializer.
TinyFile: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -o ../bin/TinyFile  -L$(BINDIR) -lsnappyc -Wl,-rpath=$(BINDIR) $(SYS_LIBS)  
client: $(OBJ2)
	$(CC) -o $@ $^ $(CFLAGS) -o ../bin/TinyClient $(SYS_LIBS)
tester:
	$(CC) tester.c -o ../bin/TinyTest
.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~