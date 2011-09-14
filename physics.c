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

void initialize() {
	paintBuffer = malloc(640 * 480 * 3 * sizeof(uint8_t));
	renderBuffer = malloc(640 * 480 * 3 * sizeof(uint8_t));
	debugBuffer = malloc(640 * 480 * 3 * sizeof(uint8_t));
	pixels = malloc(640 * 480 * sizeof(int));
	memset(pixels, -1, 640 * 480 * sizeof(int));
	drawCount = 0;
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
        case 0:
			if(rand() % 100 < 95 && (pixels[l = i + 640 + j] == -1 || pixels[l = i + j + 640 + 1] == -1 || 
			   pixels[l = i + j + 640 - 1] == -1 || pixels[l = i + j - 1] == -1 || pixels[l = i + j + 1] == -1)) {
				pixels[l] = 0;
				pixels[i + j] = -1;
			}
            break;
	}
}

void update() {
	/* Flow new particles down */
	int k = 640 / 4;
	for(int i = 0; i < 4; i++) {
		for(int a = 1; a < 3; a++) {
			for(int j = -10; j <= 10; j++) {
				if(rand() % 10 < 1) pixels[(i * k + (k / 2)) + j] = 0;
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
	for(int i = 1; i < 640 - 1; i++)
	{
		pixels[i] = -1;
		pixels[(480 - 1) * 640 + i] = -1;
	}

	for(int i = 1; i < 480 - 1; i++)
	{
		pixels[640 * i] = 1;
		pixels[(640 * (i + 1)) - 1] = 1;
	}
}

void physicsLine(int startx, int starty, int destx, int desty) {
}

void resetPhysics() {
	memset(pixels, -1, 640 * 480 * sizeof(int));
	drawCount = 0;
}

unsigned long simulate(unsigned long delta, uint8_t *walls, uint8_t *rgb) {
	/* Update walls. At the moment we just remove and replace them */
	for(int i = 0; i < 640 * 480; i++) {
		if(pixels[i] == 1 && !walls[3*i]) pixels[i] = -1;
		else if(pixels[i] == -1 && walls[3*i]) {
			pixels[i] = 1;
		}
	}
	for(int i = 0; i < 640 * 480; i++) {
		if(pixels[i] == 0 && walls[3*i]) {
			int pos = i;
			while(pos >= 0 && pixels[pos] != -1) pos -= 640;
			if(pos >= 0) pixels[pos] = 0;
			pixels[i] = 1;
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
		if(pixels[i] == 0) {
			paintBuffer[3*i+0] = 194; /* Gold colour */
			paintBuffer[3*i+1] = 178;
			paintBuffer[3*i+2] = 128;
			debugBuffer[3*i+0] = 255; /* Gold colour */
			debugBuffer[3*i+1] = 215;
			debugBuffer[3*i+2] = 0;
		}
		else if(pixels[i] == 1) {
			//paintBuffer[3*i+0] = 139; /* Gray colour */
			//paintBuffer[3*i+1] = 137;
			//paintBuffer[3*i+2] = 137;
			debugBuffer[3*i+0] = 139; /* Gray colour */
			debugBuffer[3*i+1] = 137;
			debugBuffer[3*i+2] = 137;
		}
		else {
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
