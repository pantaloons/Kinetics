#include "control.h"

#define max(a, b) ((a) < (b) ? (b) : (a))
#define abs(x) ((x) < 0 ? (-(x)) : (x))

extern pthread_mutex_t rgbBufferMutex;
extern uint16_t* backgroundDepth;
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
	IplImage *depthImage = cvGetDepth();
	
	cvAbsDiff(calibration, rgbImage, differenceImage);	
	//cvThreshold(differenceImage, differenceImage, 26, 0, CV_THRESH_TOZERO );
	
	
	for(int i=0; i < differenceImage->imageSize; i += 3) {
		if (abs(differenceImage->imageData[i+2]) + abs(differenceImage->imageData[i+1]) + abs(differenceImage->imageData[i]) >= 46) {
		//if(differenceImage->imageData[i+2] || differenceImage->imageData[i+1] || differenceImage->imageData[i]) {
			differenceImage->imageData[i+2] = 255; 	// red pixel
			differenceImage->imageData[i+1] = 255;	// green pixel
			differenceImage->imageData[i] = 255;	// blue pixel
		}
		else {
			differenceImage->imageData[i+2] = 0; 	// red pixel
			differenceImage->imageData[i+1] = 0;	// green pixel
			differenceImage->imageData[i] = 0;	// blue pixel
		}
	}
	
	cvSmooth(differenceImage, differenceImage, CV_MEDIAN, 5, 0, 0, 0);
	
	IplConvKernel* k = cvCreateStructuringElementEx(2,2,1,1,CV_SHAPE_ELLIPSE,NULL);
	//cvErode(differenceImage, differenceImage, k, 1);
    //
    cvErode(differenceImage, differenceImage, k, 1);
    //cvDilate(differenceImage, differenceImage, k, 1);
    cvReleaseStructuringElement(&k);
	
	memcpy(thresh, differenceImage->imageData, 640 * 480 * 3 * sizeof(uint8_t));
	return thresh;
}
