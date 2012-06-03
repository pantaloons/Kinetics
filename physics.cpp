#include "physics.hpp"

#define FRAMERATE 8.333

extern uint8_t walls[GAME_HEIGHT][GAME_WIDTH];
extern uint8_t colorBufs[3][GAME_HEIGHT][GAME_WIDTH][3];
extern int colorPos;

/*
 * Physics buffer -- is drawn to by the physics engine. This is double
 * buffered, so that the rendering thread can retrieve frames easily.
 */
uint8_t physicsBuffer[2][GAME_HEIGHT][GAME_WIDTH][3];
int physicsPos = 0;

pthread_mutex_t physicsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t physicsSignal = PTHREAD_COND_INITIALIZER;
int physicsUpdate = 0;

enum {
	EMPTY = 0,
	SAND,
	WALL
};

static int pixels[GAME_HEIGHT][GAME_WIDTH];
static int drawCount = 0;

void resetPhysics() {
	printf("Physics reset!\n");
	memset(pixels, EMPTY, sizeof pixels);
	drawCount = 0;
}

void swapPhysicsBuffers() {
	physicsPos = (physicsPos + 1) % 2;
	physicsUpdate = 0;
}

static void move(int x, int y) {
	static int dx[] = {0, 1, -1, -1, 1};
	static int dy[] = {1, 1,  1,  0, 0};
	switch(pixels[y][x]) {
		/* Sand propogation */
		case SAND:
			if(rand() % 100 < 95) {
				for(int i = 0; i < 5; i++) {
					if(pixels[y + dy[i]][x + dx[i]] == EMPTY) {
						pixels[y + dy[i]][x + dx[i]] = SAND;
						pixels[y][x] = EMPTY;
						break;
					}
				}
			}
			break;
		default:
			break;
	}
}

static void update() {
	/* Flow new particles down */
	int k = GAME_WIDTH / 4;
	for(int i = 0; i < 4; i++) {
		for(int a = 1; a < 3; a++) {
			for(int j = -10; j <= 10; j++) {
				if(!(rand() % 10)) pixels[0][i * k + (k / 2) + j] = SAND;
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
		pixels[0][i] = EMPTY;
		pixels[GAME_HEIGHT - 1][i] = EMPTY;
	}

	for(int i = 1; i < GAME_HEIGHT - 1; i++) {
		pixels[i][0] = WALL;
		pixels[i][GAME_WIDTH - 1] = WALL;
	}
}

unsigned long simulate(unsigned long delta) {
	/* Update walls. At the moment we just remove and replace them */
	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			if(pixels[i][j] == WALL && !walls[i][j]) pixels[i][j] = EMPTY;
			else if(pixels[i][j] == EMPTY && walls[i][j]) pixels[i][j] = WALL;
		}
	}
	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			if(pixels[i][j] == SAND && walls[i][j]) {
				int pos = i;
				while(pos >= 0 && pixels[pos][j] != EMPTY) pos--;
				if(pos >= 0) pixels[pos][j] = pixels[i][j];
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
	memcpy(physicsBuffer[(physicsPos + 1) % 2], colorBufs[colorPos], sizeof physicsBuffer[physicsPos]);
	
	/* Render sand / walls on top of buffer */
	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			if(!walls[i][j]) {
					physicsBuffer[(physicsPos + 1) % 2][i][j][0] = 0;
					physicsBuffer[(physicsPos + 1) % 2][i][j][1] = 0;
					physicsBuffer[(physicsPos + 1) % 2][i][j][2] = 0;
			}
			switch(pixels[i][j]) {
				case SAND:
					physicsBuffer[(physicsPos + 1) % 2][i][j][0] = 255;
					physicsBuffer[(physicsPos + 1) % 2][i][j][1] = 215;
					physicsBuffer[(physicsPos + 1) % 2][i][j][2] = 0;
					break;
				case WALL:
					//physicsBuffer[(physicsPos + 1) % 2][i][j][0] = 127;
					//physicsBuffer[(physicsPos + 1) % 2][i][j][1] = 127;
					//physicsBuffer[(physicsPos + 1) % 2][i][j][2] = 127;
					break;
				default:
					break;
			}
		}
	}
	
	physicsUpdate++;
	pthread_cond_signal(&physicsSignal);
	pthread_mutex_unlock(&physicsMutex);
	
	return delta;
}
