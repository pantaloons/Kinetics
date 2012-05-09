#include <inttypes.h>

#include "opencv2/opencv.hpp"

/**
 * Create the color and depth models.
 */
void createModel();

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

/**
 * Reset the model and start calibration period from.
 */
void resetModel();
