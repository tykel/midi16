CC=gcc
CFLAGS_COMMON=-std=c99 -pedantic -Wall -Wno-unused-variable -Wdeclaration-after-statement
CFLAGS=-O2 -s $(CFLAGS_COMMON)
LDFLAGS=-lm
OBJECTS=obj/main.o obj/midi.o obj/chip16.o

.PHONY: all clean debug

all: midi16

debug: CFLAGS=-O0 -g $(CFLAGS_COMMON)
debug: midi16

midi16: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

obj/main.o: src/main.c src/midi.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c  $< -o $@ $(LDFLAGS)

obj/midi.o: src/midi.c src/midi.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c  $< -o $@ $(LDFLAGS)

obj/chip16.o: src/chip16.c src/midi.h src/chip16.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c  $< -o $@ $(LDFLAGS)

clean:
	@rm -rf obj midi16
