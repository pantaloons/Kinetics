#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <pthread.h>
#include <sys/time.h>

#include "camera.h"
#include "render.h"
#include "physics.h"
#include "calibration.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define GAME_WIDTH 640
#define GAME_HEIGHT 480

extern pthread_mutex_t kinectMutex, wallMutex;
extern pthread_cond_t kinectSignal;
extern int colorUpdate, depthUpdate;

/*
 * Get the elapsed time in milliseconds.
 */
static unsigned long getTime() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return (time.tv_sec * 1000 + time.tv_usec/1000.0) + 0.5;
}

/*
 * Main engine loop. Repeatedly query for new color/depth frames
 * and send them to threshholding and then physics. Render thread
 * will pick frames up off the physics image producing queue.
 */
void *runLoop(void *arg) {
	(void)arg;
	unsigned long lastTime = getTime();
	while(1) {
		unsigned long curTime = getTime();
		unsigned long delta = curTime - lastTime;
		
		if(colorUpdate) swapColorBuffers();
		if(depthUpdate) swapDepthBuffers();

		threshhold();

		lastTime = curTime - simulate(delta);
	}
}

int main() {
	srand(time(NULL));
	if(!initCamera()) {
		fprintf(stderr, "Camera initialization failed.\n");
		return EXIT_FAILURE;
	}
	pthread_t cameraThread;
	if(pthread_create(&cameraThread, NULL, cameraLoop, NULL)) {
		fprintf(stderr, "pthread_create failed.\n");
		return EXIT_FAILURE;
	}
	
	/* We wait two frames for the camera to initialize itself. */
	for(int i = 0; i < 2; i++) {
		pthread_mutex_lock(&kinectMutex);
		while(!colorUpdate || !depthUpdate) {
			pthread_cond_wait(&kinectSignal, &kinectMutex);
		}
		pthread_mutex_unlock(&kinectMutex);
		
		swapColorBuffers();
		swapDepthBuffers();
	}
	
	createModel();

	pthread_t runThread;
	if(pthread_create(&runThread, NULL, runLoop, NULL)) {
		fprintf(stderr, "pthread_create failed.\n");
		return EXIT_FAILURE;
	}
	
	renderLoop();
	
	return EXIT_SUCCESS;
}
