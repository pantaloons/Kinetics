#include "render.h"

static void renderOne();
static void renderFour();

pthread_mutex_t glMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t physicsSignal;
int physicsUpdate;

static GLuint paintTexture;
static uint_fast8_t renderBuffer[GAME_WIDTH][GAME_HEIGHT];
static void (*paintFunc)(void) = &renderOne;

static void initScene() {
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

	glGenTextures(1, &paintTexture);
	glBindTexture(GL_TEXTURE_2D, paintTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

static void resize(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GAME_WIDTH, GAME_HEIGHT, 0, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void keyboard(unsigned char key, int x, int y) {
	if(key == 'f') calibrate();
	else if(key == '1') {
		paintFunc = &renderOne;
		glutPostRedisplay();
	}
	else if(key == '4') {
		paintFunc = &renderFour;
		glutPostRedisplay();
	}
	else if(key == 'r') resetPhysics();
}

static void render() {
	(*paintFunc)();
}

void renderLoop() {
	glutInit((int[]{0}, NULL);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	
	glutCreateWindow("Kinetics");
	
	glutDisplayFunc(&render);
	glutIdleFunc(&render);
	glutReshapeFunc(&resize);
	glutKeyboardFunc(&keyboard);

	initScene();

	glutMainLoop();
}

static void renderOne() {
	pthread_mutex_lock(&physicsMutex);
	while(!physicsUpdate) {
		pthread_cond_wait(&physicsSignal, &physicsMutex);
	}
	swapPhysicsBuffers();
	pthread_mutex_unlock(&physicsMutex);
	
	glBindTexture(GL_TEXTURE_2D, paintTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, renderBuffer);

	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glTexCoord2f(0, 0); glVertex3f(0,0,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH,0,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT,0);
	glTexCoord2f(0, 1); glVertex3f(0,WINDOW_HEIGHT,0);
	glEnd();

	glutSwapBuffers();
}

static void renderFour() {
	pthread_mutex_lock(&paintBufferMutex);
	
	while(!physicsUpdate) {
		pthread_cond_wait(&paintSignal, &paintBufferMutex);
	}
	swapPhysicsBuffers();
	
	pthread_mutex_unlock(&paintBufferMutex);
	
	glBindTexture(GL_TEXTURE_2D, paintTexture);
	
	glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, renderBuffer);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(0,0,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH,0,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT,0);
	glTexCoord2f(0, 1); glVertex3f(0,WINDOW_HEIGHT,0);
	glEnd();
	
	glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, depthImageFront);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(WINDOW_WIDTH,0,0);
	glTexCoord2f(1, 0); glVertex3f(1280,0,0);
	glTexCoord2f(1, 1); glVertex3f(1280,WINDOW_HEIGHT,0);
	glTexCoord2f(0, 1); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT,0);
	glEnd();

	//pthread_mutex_lock(&wallBufferMutex);
	//glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, wallBuffer);
	//pthread_mutex_unlock(&wallBufferMutex);
	pthread_mutex_lock(&hsvMutex);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, hsvDebug);
	pthread_mutex_unlock(&hsvMutex);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(0,WINDOW_HEIGHT,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH,960,0);
	glTexCoord2f(0, 1); glVertex3f(0,960,0);
	glEnd();
	
	
	pthread_mutex_lock(&paintBufferMutex);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, debugBuffer);
	pthread_mutex_unlock(&paintBufferMutex);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT,0);
	glTexCoord2f(1, 0); glVertex3f(1280,WINDOW_HEIGHT,0);
	glTexCoord2f(1, 1); glVertex3f(1280,960,0);
	glTexCoord2f(0, 1); glVertex3f(WINDOW_WIDTH,960,0);
	glEnd();
	
	glutSwapBuffers();
}
