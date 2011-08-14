CC = gcc
CFLAGS = -g -std=c99 -I/usr/local/include/libfreenect
LDFLAGS = -lm -lglut -lGLU -lGL -lcv -lfreenect_sync -lhighgui -lfreenect

all: kinetics

kinetics: main.o
	$(CC) $(LDFLAGS) -o $@ $^

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f main.o core.* vgcore.*
