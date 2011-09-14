#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <libfreenect_sync.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include "libfreenect.h"
#include "libfreenect_sync.h"

#include "marker.h"

int initCamera();

void swapRGBBuffers();
void swapDepthBuffers();
void swapDepthImageBuffers();

void *cameraLoop(void *arg);
void rgbFunc(freenect_device *dev, void *rgb, uint32_t timestamp);
void depthFunc(freenect_device *dev, void *v_depth, uint32_t timestamp);

IplImage *cvGetDepth();
IplImage *cvGetRGB();
