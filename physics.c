#include "physics.h"

/*
 * Paint buffer is draw to by the physics engine, swapped with the render buffer
 * which is read by opengl to draw to screen
 */
uint8_t *paintBuffer, *renderBuffer;
int physicsUpdate = 0;

extern pthread_mutex_t rgbBufferMutex;
pthread_mutex_t paintBufferMutex;
pthread_cond_t paintSignal = PTHREAD_COND_INITIALIZER;

void initialize() {
	paintBuffer = malloc(640 * 480 * 3 * sizeof(uint8_t));
	renderBuffer = malloc(640 * 480 * 3 * sizeof(uint8_t));
}

void swapPhysicsBuffers() {
	uint8_t *tmp;
	tmp = paintBuffer;
	paintBuffer = renderBuffer;
	renderBuffer = tmp;
	physicsUpdate = 0;
}

void simulate(float delta, int *walls, uint8_t *rgb) {
	//perform some physics crap
	
	pthread_mutex_lock(&paintBufferMutex);
	memcpy(paintBuffer, rgb, 640 * 480 * 3 * sizeof(uint8_t));
	//Render walls, etc on top of paintBuffer
	
	physicsUpdate++;
	pthread_cond_signal(&paintSignal);
	pthread_mutex_unlock(&paintBufferMutex);
}
