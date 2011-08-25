#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <libfreenect_sync.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include "libfreenect.h"

int initCamera();

void swapRGBBuffers();
void swapDepthBuffers();

void *cameraLoop(void *arg);
void rgbFunc(freenect_device *dev, void *rgb, uint32_t timestamp);
void depthFunc(freenect_device *dev, void *v_depth, uint32_t timestamp);

int *readCamera();
int *readSilhouette();
