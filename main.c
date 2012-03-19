#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <pthread.h>
#include <sys/time.h>

#include "libfreenect.h"

#include "camera.h"
#include "physics.h"
#include "calibration.h"
#include "control.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define GAME_WIDTH 640
#define GAME_HEIGHT 480

pthread_mutex_t kinectMutex, wallMutex;
pthread_cond_t kinectSignal;
int colorUpdate, depthUpdate;

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
static void *runLoop(void *arg) {
	unsigned long lastTime = getTime();
	while(1) {
		unsigned long curTime = getTime();
		unsigned long delta = curTime - lastTime;
		
		if(colorUpdate) swapColorBuffers();
		if(depthUpdate) swapDepthBuffers();

		updateModel(colorPos, depthPos);
		threshhold(colorPos, depthPos);

		lastTime = curTime - simulate(delta);
	}
}

int main() {
	if(!initCamera()) {
		error("Camera initialization failed.\n");
		return EXIT_FAILURE;
	}
	pthread_t cameraThread;
	if(pthread_create(&cameraThread, NULL, cameraLoop, NULL)) {
		printf("pthread_create failed.\n");
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
	
	updateModel();
	initControl();

	pthread_t runThread;
	if(pthread_create(&runThread, NULL, runLoop, NULL)) {
		printf("pthread_create failed.\n");
		return EXIT_FAILURE;
	}
	
	renderLoop();
	
	return EXIT_SUCCESS;
}
