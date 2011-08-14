CC = gcc
CFLAGS = -O3 -std=c99 -I/usr/local/include/libfreenect
LDFLAGS = -lm -lglut -lGLU -lGL -lcv -lfreenect_sync -lhighgui -lfreenect

all: kinetics

kinetics: main.o camera.o
	$(CC) $(LDFLAGS) -o $@ $^

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
camera.o: camera.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f main.o core.* vgcore.*
