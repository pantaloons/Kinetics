#include "control.h"

IplImage* differenceImage;
uint8_t* thresh;

void controlInit() {
	differenceImage = cvCreateImage(cvSize(640,480), 8, 3);
	thresh = malloc(640 * 480 * 3 * sizeof(uint8_t));
}

/*
 * Returns black and white bitmask of solids thresholded against
 * the calibration image between near and far
 * TODO: only consider values between near and far using depth map
 */
uint8_t* threshhold(IplImage* calibration, float near, float far) {
	IplImage *rgbImage = cvGetRGB();
	//diff current frame with previous frame
	cvAbsDiff(calibration, rgbImage, differenceImage);	
	//threshold difference image
	cvThreshold(differenceImage, differenceImage, 32, 0, CV_THRESH_TOZERO );
	// smooth out salt and pepper (1px) noise. smoothing after gives better walls
	cvSmooth(differenceImage, differenceImage, CV_MEDIAN, 5, 0, 0, 0);
	// 
	for(int i=0; i < differenceImage->imageSize; i+=3) {
		if (differenceImage->imageData[i+2] || differenceImage->imageData[i+1] || differenceImage->imageData[i]) {
			differenceImage->imageData[i+2] = 255; 	// red pixel
			differenceImage->imageData[i+1] = 255;	// green pixel
			differenceImage->imageData[i] = 255;	// blue pixel
		}
	}
	// binary open on difference image to close white patches in sillhouette
	//cvDilate(differenceImage, differenceImage, NULL, 7);
	//cvErode(differenceImage, differenceImage, NULL, 7);
	IplConvKernel* k = cvCreateStructuringElementEx(5,5,2,2,CV_SHAPE_ELLIPSE,NULL);
    cvErode(differenceImage, differenceImage, k, 1);
    cvReleaseStructuringElement(&k);
	
	memcpy(thresh, differenceImage->imageData, 640 * 480 * 3 * sizeof(uint8_t));
	return thresh;
}
