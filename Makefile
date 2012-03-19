CC = gcc
CFLAGS = -O2 -std=c99 -Wall -Werror -Wextra -isystem/usr/local/include/libfreenect -DGAME_WIDTH=640 -DGAME_HEIGHT=480 -DWINDOW_WIDTH=640 -DWINDOW_HEIGHT=480 -g
LDFLAGS = -lm -lglut -lGLU -lGL -lcv -lhighgui -lfreenect -lcxcore -g

all: kinetics

kinetics: main.o camera.o physics.o calibration.o render.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
render.o: render.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
calibration.o: calibration.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
camera.o: camera.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
physics.o: physics.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f *.o core.* vgcore.*
