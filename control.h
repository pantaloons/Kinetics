#include "camera.h"

/**
 * Return a boolean solid buffer representing new walls differenced
 * into the scene since calibration
 */
int* threshhold(int* calibration, float near, float far);
