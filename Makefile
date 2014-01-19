EXE := spincl
SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)
HDR := $(wildcard *.h)

CC=gcc
CFLAGS  = -Wall -DVERSION=\"`cat ./VERSION`\"

vpath %.h /usr/local/include/

$(EXE): $(OBJ)
	$(CC)  -o $@ -l rt $^ -l bcm2835

%.o: %.c $(HDR) Makefile VERSION
	$(CC) -c $(CFLAGS) -o $@ $<
	
.PHONY: clean
clean:
	rm *.o $(EXE)

.PHONY: install
install:
	sudo cp $(EXE) /opt/aeris/bin/
	sudo chmod 4755 /opt/aeris/bin/$(EXE)

