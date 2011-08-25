CC = gcc
CFLAGS = -O4 -std=c99 -isystem/usr/local/include/libfreenect
LDFLAGS = -lm -lglut -lGLU -lGL -lcv -lfreenect_sync -lhighgui -lfreenect

all: kinetics

kinetics: main.o camera.o control.o calibration.o physics.o
	$(CC) $(LDFLAGS) -o $@ $^

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
camera.o: camera.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
control.o: control.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
calibration.o: calibration.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
physics.o: physics.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f *.o core.* vgcore.*
