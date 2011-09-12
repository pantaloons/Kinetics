#include "camera.h"

void controlInit();

/**
 * Return a boolean solid buffer representing new walls differenced
 * into the scene since calibration
 */
uint8_t* threshhold(IplImage* calibration, float near, float far);
