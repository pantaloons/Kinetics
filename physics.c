#include "physics.h"

#define FRAMERATE 8.33333

uint_fast8_t colorBufs[3][GAME_WIDTH][GAME_HEIGHT][3];
int colorPos = 0;

/*
 * Physics buffer -- is drawn to by the physics engine. This is double
 * buffered, so that the rendering thread can retrieve frames easily.
 */
uint_fast8_t physicsBuffer[2][GAME_WIDTH][GAME_HEIGHT][3];
int physicsPos = 0;

pthread_mutex_t physicsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t physicsSignal = PTHREAD_COND_INITIALIZER;
int physicsUpdate = 0;

uint_fast8_t walls[GAME_WIDTH][GAME_HEIGHT];

enum {
	EMPTY,
	SAND,
	WALL
};

static int pixels[GAME_WIDTH][GAME_HEIGHT] = {EMPTY};
static int drawCount = 0;

void swapPhysicsBuffers() {
	physicsPos = (physicsPos + 1) % 2;
	physicsUpdate = 0;
}

void move(int x, int y) {
	static int dx[] = {0, 1, -1, -1, 1};
	static int dy[] = {1, 1,  1,  0, 0};
	switch(pixels[x][y]) {
		/* Sand propogation */
        case SAND:
			if(rand() % 100 < 95) {
				for(int i = 0; i < 5; i++) {
					if(pixels[x + dx[i]][y + dy[i]] == EMPTY) {
						pixels[x + dx[i]][y + dy[i]] = SAND;
						pixels[x][y] = EMPTY;
					}
				}
			}
			break;
		default:
			break;
	}
}

void update() {
	/* Flow new particles down */
	int k = GAME_WIDTH / 4;
	for(int i = 0; i < 4; i++) {
		for(int a = 1; a < 3; a++) {
			for(int j = -10; j <= 10; j++) {
				if(!(rand() % 10)) pixels[i * k + (k / 2) + j][0] = SAND;
			}
		}
	}
	/* Update existing particles */
	drawCount++;
	if(drawCount & 1) {
		for(int i = GAME_HEIGHT - 1; i >= 0; i--) {
			for(int j = 1; j < GAME_WIDTH - 1; j++) move(j, i);
		}
	}
	else {
		for(int i = GAME_HEIGHT - 1; i >= 0; i--) {
			for(int j = GAME_WIDTH - 1; j > 0; j--) move(j, i);
		}
	}
	
	/* Correct for edges */
	for(int i = 1; i < GAME_WIDTH - 1; i++) {
		pixels[i][0] = EMPTY;
		pixels[i][GAME_HEIGHT - 1] = EMPTY;
	}

	for(int i = 1; i < GAME_HEIGHT - 1; i++) {
		pixels[0][i] = WALL;
		pixels[GAME_WIDTH - 1][i] = WALL;
	}
}

void resetPhysics() {
	memset(pixels, -1, sizeof pixels);
	drawCount = 0;
}

unsigned long simulate(unsigned long delta) {
	/* Update walls. At the moment we just remove and replace them */
	for(int i = 0; i < GAME_WIDTH; i++) {
		for(int j = 0; j < GAME_HEIGHT; j++) {
			if(pixels[i][j] == WALL && !walls[i][j]) pixels[i][j] = EMPTY;
			else if(pixels[i][j] == EMPTY && walls[i][j]) pixels[i][j] = WALL;
		}
	}
	for(int i = 0; i < GAME_WIDTH; i++) {
		for(int j = 0; j < GAME_HEIGHT; j++) {
			if(pixels[i][j] == SAND && walls[i][j]) {
				int pos = j;
				while(pos >= 0 && pixels[i][pos] != EMPTY) pos--;
				if(pos >= 0) pixels[i][pos] = pixels[i][j];
				pixels[i][j] = WALL;
			}
		}
	}
	
	/* Simulate sand */
	while(delta >= FRAMERATE) {
		update();
		delta -= FRAMERATE;
	}
	
	pthread_mutex_lock(&physicsMutex);
	memcpy(physicsBuffer[physicsPos], colorBufs[colorPos], sizeof physicsBuffer[physicsPos]);
	
	/* Render sand / walls on top of buffer */
	for(int i = 0; i < GAME_WIDTH; i++) {
		for(int j = 0; j < GAME_HEIGHT; j++) {
			switch(pixels[i][j]) {
				case SAND:
					physicsBuffer[physicsPos][i][j][0] = 255;
					physicsBuffer[physicsPos][i][j][1] = 215;
					physicsBuffer[physicsPos][i][j][2] = 0;
					break;
				case WALL:
					physicsBuffer[physicsPos][i][j][0] = 139;
					physicsBuffer[physicsPos][i][j][1] = 137;
					physicsBuffer[physicsPos][i][j][2] = 137;
					break;
				default:
					physicsBuffer[physicsPos][i][j][0] = 0;
					physicsBuffer[physicsPos][i][j][1] = 0;
					physicsBuffer[physicsPos][i][j][2] = 0;
					break;
			}
		}
	}
	
	physicsUpdate++;
	pthread_cond_signal(&physicsSignal);
	pthread_mutex_unlock(&physicsMutex);
	
	return delta;
}
