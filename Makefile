CC=gcc
CFLAGS=-O0 -g -std=c99
LDFLAGS=
OBJECTS=obj/main.o obj/midi.o

.PHONY: all clean

all: midi16

midi16: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

obj/main.o: src/main.c src/midi.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c  $< -o $@ $(LDFLAGS)

obj/midi.o: src/midi.c src/midi.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c  $< -o $@ $(LDFLAGS)

clean:
	@rm -rf obj midi16
