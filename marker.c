#include "marker.h"


#define HEIGHT 480
#define WIDTH 640

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX3(a, b, c) (MAX(MAX(a, b), c))in
#define MIN3(a, b, c) (MIN(MIN(a, b), c))

/* HSV conversion code based on http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_(C) */

uint8_t *hsvDebug;
pthread_mutex_t hsvMutex;

int rgbToHue2(int ri, int gi, int bi) {
	double r = ri / 255.0f;
	double g = gi / 255.0f;
	double b = bi / 255.0f;
	double max = MAX(r, MAX(g, b));
	double min = MIN(r, MIN(g, b));
	double d = max - min;
	double h = 0;
	if(d < 1e-8) return h;
	else {
		if(max - r < 1e-8) h = (g - b) / d + (g < b ? 6 : 0);
		else if(max - g < 1e-8) h = (b - r) / d + 2;
		else if(max - b < 1e-8) h = (r - g) / d + 4;
	}
	h /= 6.0f;
	return (int)(h * 255);
}

IplImage* GetThresholdedImage(IplImage* img)
{
	// Convert the image into an HSV image
	IplImage* imgHSV = cvCreateImage(cvGetSize(img), 8, 3);
	cvCvtColor(img, imgHSV, CV_RGB2HSV);


	IplImage* imgThreshed = cvCreateImage(cvGetSize(img), 8, 1);


	// Values 20,100,100 to 30,255,255 working perfect for yellow at around 6pm
	cvInRangeS(imgHSV, cvScalar(22, 100, 100,1), cvScalar(28, 225, 200,1), imgThreshed);
	
	
	IplConvKernel* k = cvCreateStructuringElementEx(3,3,1,1,CV_SHAPE_ELLIPSE,NULL);
	cvErode(imgThreshed, imgThreshed, k, 1);
    cvReleaseStructuringElement(&k);
	
	pthread_mutex_lock(&hsvMutex);
		
	for(int i = 0; i < 640 * 480; i++) {
		//int val = rgbToHue2(rgb[i*3], rgb[i*3+1], rgb[i*3+2]);
		int val = imgThreshed->imageData[i];
		//printf("Image data: %i \n",imgThreshed->imageData[i]);
		if(val == -1){//>= hue - 5 && val <= hue + 5) {
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
	pthread_mutex_unlock(&hsvMutex);

	cvReleaseImage(&imgHSV);

	return imgThreshed;
}


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
	
	printf("Hue %d\n", rgbToHue2(rgb[0], rgb[1], rgb[2]));

	
	IplImage* img2=cvCreateImageHeader(cvSize(640,480), 8, 3);
	cvSetData(img2, rgb, 640*3);
	
	// Holds the yellow thresholded image (yellow = white, rest = black)
	IplImage* imgYellowThresh = GetThresholdedImage(img2);
		
	
	// Calculate the mouments to estimate the position of the ball
	CvMoments *moments = (CvMoments*)malloc(sizeof(CvMoments));
	cvMoments(imgYellowThresh, moments, 1);

	// The actual moment values
	double moment10 = cvGetSpatialMoment(moments, 1, 0);
	double moment01 = cvGetSpatialMoment(moments, 0, 1);
	double area = cvGetCentralMoment(moments, 0, 0);

	// Holding the last and current ball positions
	*outx = moment10/area;
	*outy = moment01/area;
	
	
	CvSize size;
	
	//cvGetRawData(imgYellowThresh,output,8,&size);
	
	//return 0;
	printf("Found marker at position: %d, %d\n", *outx, *outy);
	
	return 1;
}
