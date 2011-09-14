CC = gcc
CFLAGS = -O4 -std=c99 -isystem/usr/local/include/libfreenect
LDFLAGS = -lm -lglut -lGLU -lGL -lcv -lhighgui -lfreenect

all: kinetics

kinetics: main.o camera.o physics.o calibration.o control.o marker.o
	$(CC) $(LDFLAGS) -o $@ $^

main.o: main.c camera.o physics.o calibration.o control.o
	$(CC) $(CFLAGS) -c -o $@ $<
	
calibration.o: calibration.c camera.o
	$(CC) $(CFLAGS) -c -o $@ $<
	
control.o: control.c camera.o
	$(CC) $(CFLAGS) -c -o $@ $<
	
camera.o: camera.c physics.o marker.o
	$(CC) $(CFLAGS) -c -o $@ $<
	
physics.o: physics.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
marker.o: marker.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f *.o core.* vgcore.*
