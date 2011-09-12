#include "calibration.h"

void calibrate(float near, float far, IplImage* result) {
	IplImage *rgb = cvGetRGB();
	// copies rgb to result
	cvCopy(rgb, result, NULL);
	printf("copied frame to calibration\n");
}
