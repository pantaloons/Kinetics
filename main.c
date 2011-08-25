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

pthread_t runThread;
pthread_t cameraThread;

GLuint rgbTexture;

extern uint8_t *renderBuffer, *rgbFront;
extern int rgbUpdate, physicsUpdate;

extern pthread_mutex_t paintBufferMutex;
extern pthread_cond_t paintSignal;

int* calibration;
float near = 1.0f;
float far = 100.0f;

void renderLoop(int argc, char** argv);
void *runLoop(void *arg);

int main(int argc, char** argv) {
	if(initCamera()) {
		printf("camera initialization failed!\n");
		return 1;
	}
	int result = pthread_create(&cameraThread, NULL, cameraLoop, NULL);
	if(result) {
		printf("pthread_create() failed\n");
		return 1;
	}
	
	calibration = calibrate(near, far);
	
	initialize();
	
	result = pthread_create(&runThread, NULL, runLoop, NULL);
	if(result) {
		printf("pthread_create() failed\n");
		return 1;
	}
	
	printf("Hello!\n");
	renderLoop(argc, argv);

	return 0;
}

unsigned long getTime() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return (time.tv_sec * 1000 + time.tv_usec/1000.0) + 0.5;
}

void *runLoop(void *arg) {
	unsigned long lastTime = getTime();
	while(1) {
		unsigned long curTime = getTime();
		unsigned long delta = curTime - lastTime;
		
		if(rgbUpdate) swapRGBBuffers();
		
		int* walls = threshhold(calibration, near, far);
		simulate(delta, walls, rgbFront);
		
		lastTime = curTime;
	}
}

void initScene() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_FLAT);

	glGenTextures(1, &rgbTexture);
	glBindTexture(GL_TEXTURE_2D, rgbTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void resize(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void keyboard(unsigned char key, int x, int y) {
	
}

void render() {
	pthread_mutex_lock(&paintBufferMutex);
	
	while(!physicsUpdate) {
		pthread_cond_wait(&paintSignal, &paintBufferMutex);
	}
	swapPhysicsBuffers();
	
	pthread_mutex_unlock(&paintBufferMutex);
	
	glBindTexture(GL_TEXTURE_2D, rgbTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, renderBuffer);

	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glTexCoord2f(0, 0); glVertex3f(0,0,0);
	glTexCoord2f(1, 0); glVertex3f(640,0,0);
	glTexCoord2f(1, 1); glVertex3f(640,480,0);
	glTexCoord2f(0, 1); glVertex3f(0,480,0);
	glEnd();

	glutSwapBuffers();
}

void renderLoop(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
	glutInitWindowSize(640, 480);
	glutInitWindowPosition(0, 0);
	
	glutCreateWindow("Kinetics");
	
	glutDisplayFunc(&render);
	glutIdleFunc(&render);
	glutReshapeFunc(&resize);
	glutKeyboardFunc(&keyboard);

	initScene();

	glutMainLoop();
}

