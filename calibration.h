#include "camera.h"

/**
 * Return a pixel depth buffer representing an initial
 * baseline camera calibration to be differenced against
 * in future.
 */
void calibrate(float near, float far, IplImage* result);
