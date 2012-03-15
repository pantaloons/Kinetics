#include "physics.h"

/*
 * Paint buffer is draw to by the physics engine, swapped with the render buffer
 * which is read by opengl to draw to screen
 */
uint8_t *paintBuffer, *renderBuffer, *debugBuffer;
int physicsUpdate = 0;

extern pthread_mutex_t rgbBufferMutex;
pthread_mutex_t paintBufferMutex;
pthread_cond_t paintSignal = PTHREAD_COND_INITIALIZER;

int *pixels;
int drawCount;
int frameRate = 1000/120; /* Milliseconds per frame */
int inited = 0;

#define EMPTY -1
#define SAND 0
#define WALL 1
#define PLANT 2
#define WATER 3

void initialize() {
	paintBuffer = malloc(640 * 480 * 3 * sizeof(uint8_t));
	renderBuffer = malloc(640 * 480 * 3 * sizeof(uint8_t));
	debugBuffer = malloc(640 * 480 * 3 * sizeof(uint8_t));
	pixels = malloc(640 * 480 * sizeof(int));
	memset(pixels, -1, 640 * 480 * sizeof(int));
	drawCount = 0;
	inited = 1;
}

void swapPhysicsBuffers() {
	uint8_t *tmp;
	tmp = paintBuffer;
	paintBuffer = renderBuffer;
	renderBuffer = tmp;
	physicsUpdate = 0;
}

void move(int i, int j, int k) {
	int l;
	switch(pixels[i + j]) {
		/* Sand propogation */
        case SAND:
			if(rand() % 100 < 95 && (pixels[l = i + 640 + j] == EMPTY || pixels[l = i + j + 640 + 1] == EMPTY || 
			   pixels[l = i + j + 640 - 1] == EMPTY || pixels[l = i + j - 1] == EMPTY || pixels[l = i + j + 1] == EMPTY)) {
				pixels[l] = SAND;
				pixels[i + j] = EMPTY;
			}
            break;
        case WATER:
			if(rand() % 100 < 95 && (pixels[l = i + 640 + j] == EMPTY || pixels[l = i + j + 640 + 1] == EMPTY || 
			   pixels[l = i + j + 640 - 1] == EMPTY || pixels[l = i + j - 1] == EMPTY || pixels[l = i + j + 1] == EMPTY)) {
				pixels[l] = WATER;
				pixels[i + j] = EMPTY;
			}
			break;
		case PLANT:
            if(rand() % 100 >= 10) break;
            if(pixels[l = (i - 640) + j] == WATER || pixels[l = i + 640 + j] == WATER ||
			   pixels[l = i + 1 + j] == WATER || pixels[l = (i - 1) + j] == WATER) {
                pixels[l] = PLANT;
			}
            break;
	}
}

void update() {
	/* Flow new particles down */
	int k = 640 / 4;
	for(int i = 0; i < 4; i+=2) {
		for(int a = 1; a < 3; a++) {
			for(int j = -10; j <= 10; j++) {
				if(rand() % 10 < 1) pixels[(i * k + (k / 2)) + j] = SAND;
			}
		}
	}
	for(int i = 1; i < 4; i+=2) {
		for(int a = 1; a < 3; a++) {
			for(int j = -10; j <= 10; j++) {
				if(rand() % 10 < 1) pixels[(i * k + (k / 2)) + j] = WATER;
			}
		}
	}
	/* Update existing particles */
	drawCount++;
	if(drawCount & 1) {
		for(int i = 480 - 1; i >= 0; i--) {
			int pos = i * 640;
			for(int j = 0; j < 640 - 1; j++) move(pos, j, i);
		}
	}
	else {
		for(int i = 480 - 1; i >= 0; i--) {
			int pos = i * 640;
			for(int j = 640 - 1; j >= 0; j--) move(pos, j, i);
		}
	}
	
	/* Correct for edges */
	for(int i = 1; i < 640 - 1; i++) {
		pixels[i] = EMPTY;
		pixels[(480 - 1) * 640 + i] = EMPTY;
	}

	for(int i = 1; i < 480 - 1; i++) {
		pixels[640 * i] = WALL;
		pixels[(640 * (i + 1)) - 1] = WALL;
	}
}

void resetPhysics() {
	memset(pixels, -1, 640 * 480 * sizeof(int));
	drawCount = 0;
}

unsigned long simulate(unsigned long delta, uint8_t *walls, uint8_t *rgb) {
	/* Update walls. At the moment we just remove and replace them */
	for(int i = 0; i < 640 * 480; i++) {
		if(pixels[i] == WALL && !walls[3*i]) pixels[i] = EMPTY;
		else if(pixels[i] == EMPTY && walls[3*i]) {
			pixels[i] = WALL;
		}
	}
	for(int i = 0; i < 640 * 480; i++) {
		if((pixels[i] == SAND || pixels[i] == WATER) && walls[3*i]) {
			int pos = i;
			while(pos >= 0 && pixels[pos] != EMPTY) pos -= 640;
			if(pos >= 0) pixels[pos] = pixels[i];
			pixels[i] = WALL;
		}
	}
	/* Simulate sand */
	while(delta >= frameRate) {
		update();
		delta -= frameRate;
	}
	
	pthread_mutex_lock(&paintBufferMutex);
	memcpy(paintBuffer, rgb, 640 * 480 * 3 * sizeof(uint8_t));
	
	/* Render sand / walls on top of image */
	for(int i = 0; i < 640 * 480; i++) {
		if(pixels[i] == SAND) {
			paintBuffer[3*i+0] = 255; /* Gold colour */
			paintBuffer[3*i+1] = 215;
			paintBuffer[3*i+2] = 0;
			debugBuffer[3*i+0] = 255; /* Gold colour */
			debugBuffer[3*i+1] = 215;
			debugBuffer[3*i+2] = 0;
		}
		else if(pixels[i] == WATER) {
			paintBuffer[3*i+0] = 0; /* Blue colour */
			paintBuffer[3*i+1] = 0;
			paintBuffer[3*i+2] = 137;
			debugBuffer[3*i+0] = 0; /* Blue colour */
			debugBuffer[3*i+1] = 0;
			debugBuffer[3*i+2] = 137;
		}
		else if(pixels[i] == PLANT) {
			paintBuffer[3*i+0] = 0; /* Gray colour */
			paintBuffer[3*i+1] = 137;
			paintBuffer[3*i+2] = 0;
			debugBuffer[3*i+0] = 0; /* Green colour */
			debugBuffer[3*i+1] = 137;
			debugBuffer[3*i+2] = 0;
		}
		else if(pixels[i] == WALL) {
			debugBuffer[3*i+0] = 139; /* Gray colour */
			debugBuffer[3*i+1] = 137;
			debugBuffer[3*i+2] = 137;
		}
		else { /* Empty */
			debugBuffer[3*i+0] = 0; /* Black colour */
			debugBuffer[3*i+1] = 0;
			debugBuffer[3*i+2] = 0;
		}
	}
	
	physicsUpdate++;
	pthread_cond_signal(&paintSignal);
	pthread_mutex_unlock(&paintBufferMutex);
	
	return delta;
}
