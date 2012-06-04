#include "render.hpp"
#include "physics.hpp"
#include "calibration.hpp"

static void renderOne();

extern uint8_t physicsBuffer[2][GAME_HEIGHT][GAME_WIDTH][3];
extern int physicsPos;
extern pthread_mutex_t physicsMutex;
extern pthread_cond_t physicsSignal;
extern int physicsUpdate;

static GLuint paintTexture;
static void (*paintFunc)(void) = &renderOne;

/* Debug Buffers */
#ifdef DEBUG
extern uint8_t colorBufs[3][GAME_HEIGHT][GAME_WIDTH][3];
extern int colorPos;
extern uint16_t depthBufs[2][GAME_HEIGHT][GAME_WIDTH];
extern int depthPos;
extern uint8_t walls[GAME_HEIGHT][GAME_WIDTH];
extern uint8_t colorFGDebug[GAME_HEIGHT][GAME_WIDTH];
extern uint8_t depthFGDebug[GAME_HEIGHT][GAME_WIDTH];
extern uint8_t colorFGDebugFilt[GAME_HEIGHT][GAME_WIDTH];
extern uint8_t depthFGDebugFilt[GAME_HEIGHT][GAME_WIDTH];
extern uint8_t finalBefore[GAME_HEIGHT][GAME_WIDTH];
static uint8_t depthDebug[GAME_HEIGHT][GAME_WIDTH][3];
static void renderNine();
void someDepthFunc();
#endif

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
#ifdef DEBUG
	else if(key == '9') {
		paintFunc = &renderNine;
		glutPostRedisplay();
	}
#endif
	else if(key == 'r') resetPhysics();
	else if(key == 'f') resetModel();
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
	glTexImage2D(GL_TEXTURE_2D, 0, 3, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, physicsBuffer[physicsPos]);

	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH, 0, 0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	glTexCoord2f(0, 1); glVertex3f(0, WINDOW_HEIGHT, 0);
	glEnd();

	glutSwapBuffers();
}

#ifdef DEBUG
static void renderNine() {
	pthread_mutex_lock(&physicsMutex);
	while(!physicsUpdate) {
		pthread_cond_wait(&physicsSignal, &physicsMutex);
	}
	swapPhysicsBuffers();
	pthread_mutex_unlock(&physicsMutex);
	
	glBindTexture(GL_TEXTURE_2D, paintTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, colorBufs[colorPos]);
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH / 3, 0, 0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH / 3, WINDOW_HEIGHT / 3, 0);
	glTexCoord2f(0, 1); glVertex3f(0, WINDOW_HEIGHT / 3, 0);
	glEnd();
	
	someDepthFunc();
	glTexImage2D(GL_TEXTURE_2D, 0, 3, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, depthDebug);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(WINDOW_WIDTH / 3, 0, 0);
	glTexCoord2f(1, 0); glVertex3f(2 * WINDOW_WIDTH / 3, 0, 0);
	glTexCoord2f(1, 1); glVertex3f(2 * WINDOW_WIDTH / 3, WINDOW_HEIGHT / 3, 0);
	glTexCoord2f(0, 1); glVertex3f(WINDOW_WIDTH / 3, WINDOW_HEIGHT / 3, 0);
	glEnd();

	glTexImage2D(GL_TEXTURE_2D, 0, 1, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, colorFGDebug);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(0, WINDOW_HEIGHT/3,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH/3,WINDOW_HEIGHT/3,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH/3,2*WINDOW_HEIGHT/3,0);
	glTexCoord2f(0, 1); glVertex3f(0,2*WINDOW_HEIGHT/3,0);
	glEnd();
	
	glTexImage2D(GL_TEXTURE_2D, 0, 3, GAME_WIDTH, GAME_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, depthFGDebug);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(WINDOW_WIDTH/3, WINDOW_HEIGHT/3, 0);
	glTexCoord2f(1, 0); glVertex3f(2*WINDOW_WIDTH/3, WINDOW_HEIGHT/3, 0);
	glTexCoord2f(1, 1); glVertex3f(2*WINDOW_WIDTH/3, 2*WINDOW_HEIGHT/3, 0);
	glTexCoord2f(0, 1); glVertex3f(WINDOW_WIDTH/3, 2*WINDOW_HEIGHT/3, 0);
	glEnd();
	
	glTexImage2D(GL_TEXTURE_2D, 0, 1, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, colorFGDebugFilt);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(0, 2*WINDOW_HEIGHT/3,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH/3,2*WINDOW_HEIGHT/3,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH/3,3*WINDOW_HEIGHT/3,0);
	glTexCoord2f(0, 1); glVertex3f(0,3*WINDOW_HEIGHT/3,0);
	glEnd();
	
	glTexImage2D(GL_TEXTURE_2D, 0, 3, GAME_WIDTH, GAME_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, depthFGDebugFilt);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(WINDOW_WIDTH/3, 2*WINDOW_HEIGHT/3, 0);
	glTexCoord2f(1, 0); glVertex3f(2*WINDOW_WIDTH/3, 2*WINDOW_HEIGHT/3, 0);
	glTexCoord2f(1, 1); glVertex3f(2*WINDOW_WIDTH/3, 3*WINDOW_HEIGHT/3, 0);
	glTexCoord2f(0, 1); glVertex3f(WINDOW_WIDTH/3, 3*WINDOW_HEIGHT/3, 0);
	glEnd();
	
	glTexImage2D(GL_TEXTURE_2D, 0, 1, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, finalBefore);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(2*WINDOW_WIDTH/3, 0,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH, 0,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH,WINDOW_HEIGHT/3,0);
	glTexCoord2f(0, 1); glVertex3f(2*WINDOW_WIDTH/3,WINDOW_HEIGHT/3,0);
	glEnd();
	
	glTexImage2D(GL_TEXTURE_2D, 0, 1, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, walls);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(2*WINDOW_WIDTH/3, WINDOW_HEIGHT/3,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH, WINDOW_HEIGHT/3,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH,2*WINDOW_HEIGHT/3,0);
	glTexCoord2f(0, 1); glVertex3f(2*WINDOW_WIDTH/3,2*WINDOW_HEIGHT/3,0);
	glEnd();
	
	glTexImage2D(GL_TEXTURE_2D, 0, 3, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, physicsBuffer[physicsPos]);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex3f(2*WINDOW_WIDTH/3, 2*WINDOW_HEIGHT/3,0);
	glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH, 2*WINDOW_HEIGHT/3,0);
	glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH,3*WINDOW_HEIGHT/3,0);
	glTexCoord2f(0, 1); glVertex3f(2*WINDOW_WIDTH/3,3*WINDOW_HEIGHT/3,0);
	glEnd();

	glutSwapBuffers();
}

void someDepthFunc() {
	static uint16_t t_gamma[10000];
	static bool doGamma = true;
	if(doGamma) {
		for(int i = 0; i < 10000; i++) {
			float v = i/2048.0;
			v = powf(v, 3)* 6;
			t_gamma[i] = v*6*256;
		}
	}


	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			int pval = t_gamma[depthBufs[depthPos][i][j]] / 4;
			int lb = pval & 0xff;
			
			if(depthBufs[depthPos][i][j] == 0) {
				depthDebug[i][j][0] = 0;
				depthDebug[i][j][1] = 0;
				depthDebug[i][j][2] = 0;
			}
			switch (pval >> 8) {
				case 0:
					depthDebug[i][j][0] = 255;
					depthDebug[i][j][1] = 255 - lb;
					depthDebug[i][j][2] = 255 - lb;
					break;
				case 1:
					depthDebug[i][j][0] = 255;
					depthDebug[i][j][1] = lb;
					depthDebug[i][j][2] = 0;
					break;
				case 2:
					depthDebug[i][j][0] = 255 - lb;
					depthDebug[i][j][1] = 255;
					depthDebug[i][j][2] = 0;
					break;
				case 3:
					depthDebug[i][j][0] = 0;
					depthDebug[i][j][1] = 255;
					depthDebug[i][j][2] = lb;
					break;
				case 4:
					depthDebug[i][j][0] = 0;
					depthDebug[i][j][1] = 255 - lb;
					depthDebug[i][j][2] = 255;
					break;
				case 5:
					depthDebug[i][j][0] = 0;
					depthDebug[i][j][1] = 0;
					depthDebug[i][j][2] = 255 - lb;
					break;
				default:
					depthDebug[i][j][0] = 0;
					depthDebug[i][j][1] = 0;
					depthDebug[i][j][2] = 0;
					break;
			}
		}
	}
}
#endif
