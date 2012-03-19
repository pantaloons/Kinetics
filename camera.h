#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

//#include "libfreenect.h"
//#include "libfreenect_sync.h"

bool initCamera();
void swapColorBuffers();
void swapDepthBuffers();
void *cameraLoop(void *arg);
