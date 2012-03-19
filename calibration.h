#include <inttypes.h>

#include "opencv/cv.h"

/**
 * Update the color and depth models to account for a new
 * reading.
 */
void updateModel();

/**
 * Updates black and white bitmask of solids thresholded against
 * the calibration model.
 */
void threshhold();
