#include "marker.h"

#define HEIGHT 480
#define WIDTH 640

/* HSV conversion code based on http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_(C) */

uint8_t *hsvDebug;
pthread_mutex_t hsvMutex;

/* Visited array for flood fill */
int visited[640 * 480];
int queue[640 * 480];
int list[640 * 480];
int qindex = 0;
int lindex = 0;

CvMoments* moments;
IplImage* img2;
IplImage* imgThreshed;
IplImage* imgHSV;

void markerInit(void)
{
	moments = (CvMoments*)malloc(sizeof(CvMoments));
	img2 = cvCreateImageHeader(cvSize(640,480), 8, 3);
	imgThreshed = cvCreateImage(cvGetSize(img2), 8, 1);
	imgHSV = cvCreateImage(cvGetSize(img2), 8, 3);
}

void floodFill(uint8_t *image) {
	memset(visited, 0, sizeof(visited));
	qindex = 0;
	for(int i = 0; i < 640 * 480; i++) {
		if(visited[i] || image[i] == 0) continue;
		int count = 0;
		lindex = 0;
		queue[qindex++] = i;
		list[lindex++] = i;
		while(qindex > 0) {
			int pos = queue[--qindex];
			count++;
			int x = pos % 640;
			int y = pos / 640;
			for(int j = -5; j <= 5; j++) {
				for(int k = -5; k <= 5; k++) {
					if(j * j + k * k > 25) continue;
					int nx = x + j;
					int ny = y + k;
					if(nx < 0 || ny < 0 || nx >= 640 || ny >= 480) continue;
					if(visited[nx + ny * 640] || image[nx + ny * 640] == 0) continue;
					queue[qindex++] = nx + ny * 640;
					list[lindex++] = nx + ny * 640;
					visited[nx + ny * 640] = 1;
				}
			}
		}
		if(count < 15) {
			for(int j = 0; j < count; j++) image[list[j]] = 0;
		}
	}
}

void GetThresholdedImage(IplImage* img, int hue)
{
	// Convert the image into an HSV image
	
	cvCvtColor(img, imgHSV, CV_RGB2HSV);
	
	// Values 20,100,100 to 30,255,255 working perfect for yellow at around 6pm
	cvInRangeS(imgHSV, cvScalar(hue-5, 100, 100,1), cvScalar(hue+5, 200, 200,1), imgThreshed);
	
	
	
	IplConvKernel* k = cvCreateStructuringElementEx(3,3,1,1,CV_SHAPE_ELLIPSE,NULL);
	cvErode(imgThreshed, imgThreshed, k, 1);
    cvReleaseStructuringElement(&k);
    
    //floodFill(imgThreshed->imageData);
	
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
	cvSetData(img2, rgb, 640*3);
	
	// Holds the yellow thresholded image (yellow = white, rest = black)
	GetThresholdedImage(img2,hue);
	
		
	
	// Calculate the mouments to estimate the position of the ball
	cvMoments(imgThreshed, moments, 1);

	// The actual moment values
	double moment10 = cvGetSpatialMoment(moments, 1, 0);
	double moment01 = cvGetSpatialMoment(moments, 0, 1);
	double area = cvGetCentralMoment(moments, 0, 0);

	// Holding the last and current ball positions
	*outx = moment10/area;
	*outy = moment01/area;
		
	if(area < 50){//*outx < 1 || *outy < 1){
		
		return 0;
		
	}
	//printf("Found marker at position: %d, %d\n", *outx, *outy);
	
	return 1;
}
