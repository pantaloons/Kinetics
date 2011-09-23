#include "marker.h"

#define HEIGHT 480
#define WIDTH 640

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX3(a, b, c) (MAX(MAX(a, b), c))
#define MIN3(a, b, c) (MIN(MIN(a, b), c))

/* HSV conversion code based on http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_(C) */

uint8_t *hsvDebug;
pthread_mutex_t hsvMutex;


/**
 * Locate the blue marker in the image
 * (r, g, b) specifies the marker colour (approx), rgb* is the image array
 * to search in, and outx, outy to be written to if found
 */
int findMarker(int hue, uint8_t* rgb, int* outx, int* outy) {
	/* Locate the marker in the rgb image (640x480),
	 * return 0 if it is not found, if it is found
	 * return 1 and save the x and y position (center)
	 * to outx, outy */
	
	/*
	pthread_mutex_lock(&hsvMutex);
	for(int i = 0; i < 640 * 480; i++) {
		if(output[i]) {
			hsvDebug[i*3] = 255;
			hsvDebug[i*3+1] = 255;
			hsvDebug[i*3+2] = 255;
		}
		else {
			hsvDebug[i*3] = 0;
			hsvDebug[i*3+1] = 0;
			hsvDebug[i*3+2] = 0;
		}
	}
	pthread_mutex_unlock(&hsvMutex);*/

	return 0;
	printf("Found marker at position: %d, %d\n", *outx, *outy);
	
	return 1;
}
