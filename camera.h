#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "libfreenect.h"

bool initCamera();
void swapColorBuffers();
void swapDepthBuffers();
void *cameraLoop(void *arg);
