#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <opencv/cv.h>


/**
 * Locate the blue marker in the image
 */
int findMarker(int hue, uint8_t* rgb, int* outx, int* outy);
