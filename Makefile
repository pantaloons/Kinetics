CC = g++
CFLAGS = -O3 -Wall -Werror -Wextra -isystem/usr/local/include/libfreenect -DGAME_WIDTH=640 -DGAME_HEIGHT=480 -DWINDOW_WIDTH=640 -DWINDOW_HEIGHT=480 -DDEBUG
LDFLAGS = -O3 -lpthread -lm -lglut -lGLU -lGL -lopencv_core -lopencv_video -lopencv_imgproc -lopencv_highgui -lfreenect

all: kinetics

kinetics: main.o camera.o physics.o calibration.o render.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
render.o: render.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
calibration.o: calibration.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
camera.o: camera.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
physics.o: physics.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f *.o core.* vgcore.*
