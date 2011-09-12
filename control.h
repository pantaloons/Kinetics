#include "camera.h"

void controlInit();

/**
 * Return a boolean solid buffer representing new walls differenced
 * into the scene since calibration
 */
int* threshhold(IplImage* calibration, float near, float far);
