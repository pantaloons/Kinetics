#include "render.h"
#include "physics.h"

static void renderOne();
static void renderFour();

extern pthread_mutex_t physicsMutex;
extern pthread_cond_t physicsSignal;
extern int physicsUpdate;

static GLuint paintTexture;
static uint8_t renderBuffer[GAME_WIDTH][GAME_HEIGHT];
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
	(void)x, (void)y;
	/* if(key == 'f') calibrate(); */
	if(key == '1') {
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
	glutInit((int[]){0}, NULL);
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
	pthread_mutex_lock(&physicsMutex);
	
	while(!physicsUpdate) {
		pthread_cond_wait(&physicsSignal, &physicsMutex);
	}
	swapPhysicsBuffers();
	
	pthread_mutex_unlock(&physicsMutex);
	
	glBindTexture(GL_TEXTURE_2D, paintTexture);
	
	glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, renderBuffer);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(0,0,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH,0,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT,0);
	glTexCoord2f(0, 1); glVertex3f(0,WINDOW_HEIGHT,0);
	glEnd();
	
	/*
	glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, depthImageFront);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(WINDOW_WIDTH,0,0);
	glTexCoord2f(1, 0); glVertex3f(1280,0,0);
	glTexCoord2f(1, 1); glVertex3f(1280,WINDOW_HEIGHT,0);
	glTexCoord2f(0, 1); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT,0);
	glEnd();
	*/
	
	//pthread_mutex_lock(&wallBufferMutex);
	//glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, wallBuffer);
	//pthread_mutex_unlock(&wallBufferMutex);
	/*pthread_mutex_lock(&hsvMutex);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, hsvDebug);
	pthread_mutex_unlock(&hsvMutex);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(0,WINDOW_HEIGHT,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH,960,0);
	glTexCoord2f(0, 1); glVertex3f(0,960,0);
	glEnd();*/
	
	/*
	pthread_mutex_lock(&paintBufferMutex);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, debugBuffer);
	pthread_mutex_unlock(&paintBufferMutex);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT,0);
	glTexCoord2f(1, 0); glVertex3f(1280,WINDOW_HEIGHT,0);
	glTexCoord2f(1, 1); glVertex3f(1280,960,0);
	glTexCoord2f(0, 1); glVertex3f(WINDOW_WIDTH,960,0);
	glEnd();
	*/
	
	glutSwapBuffers();
}

void someDepthFunc() {
	static uint16_t gamma[2048];
	static bool doGamma = true;
	if(doGamma) {
		/* Black magic... this converts kinect depth values to real distance */
		for (int i = 0; i < 2048; i++) {
				float v = i/2048.0;
				v = powf(v, 3) * 6;
				gamma[i] = v * 6 * 256;
		}
		doGamma = false;
	}
	/* todo: use gamma */
	(void)gamma;
	/*
	for(int i = 0; i < GAME_WIDTH * GAME_HEIGHT; i++) {
		depthStage[i] = depth[i];
		int pval = t_gamma_i[depth[i]];
		
		int lb = pval & 0xff;
		switch (pval >> 8) {
			case 0:
				depthImageStage[3*i+0] = 255;
				depthImageStage[3*i+1] = 255 - lb;
				depthImageStage[3*i+2] = 255 - lb;
				break;
			case 1:
				depthImageStage[3*i+0] = 255;
				depthImageStage[3*i+1] = lb;
				depthImageStage[3*i+2] = 0;
				break;
			case 2:
				depthImageStage[3*i+0] = 255 - lb;
				depthImageStage[3*i+1] = 255;
				depthImageStage[3*i+2] = 0;
				break;
			case 3:
				depthImageStage[3*i+0] = 0;
				depthImageStage[3*i+1] = 255;
				depthImageStage[3*i+2] = lb;
				break;
			case 4:
				depthImageStage[3*i+0] = 0;
				depthImageStage[3*i+1] = 255 - lb;
				depthImageStage[3*i+2] = 255;
				break;
			case 5:
				depthImageStage[3*i+0] = 0;
				depthImageStage[3*i+1] = 0;
				depthImageStage[3*i+2] = 255 - lb;
				break;
			default:
				depthImageStage[3*i+0] = 0;
				depthImageStage[3*i+1] = 0;
				depthImageStage[3*i+2] = 0;
				break;
		}
	}*/
}
