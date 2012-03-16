CC = gcc
CFLAGS = -O4 -std=c99 -isystem/usr/local/include/libfreenect -DGAME_WIDTH=640 -DGAME_HEIGHT=480
LDFLAGS = -lm -lglut -lGLU -lGL -lcv -lhighgui -lfreenect -lcxcore

all: kinetics

kinetics: main.o camera.o physics.o calibration.o control.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c camera.o physics.o calibration.o control.o
	$(CC) $(CFLAGS) -c -o $@ $<
	
render.o: render.c physics.o
	$(CC) $(CFLAGS) -c -o $@ $<
	
calibration.o: calibration.c camera.o
	$(CC) $(CFLAGS) -c -o $@ $<
	
camera.o: camera.c physics.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
physics.o: camera.c physics.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f *.o core.* vgcore.*
