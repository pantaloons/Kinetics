#include "calibration.h"

void calibrate(float near, float far, IplImage* result) {
	IplImage *rgb = cvGetRGB();
	cvCopy(rgb, result, NULL);
	printf("copied frame to calibration\n");
}
