#include "camera.h"

/**
 * Note, this doesn't work... it's example for something else
 */
void updateCamera() { 
      IplImage *image = cvCreateImageHeader(cvSize(640,480), 8, 3);
      while (cvWaitKey(10) < 0) 
      {
        char *data;
        unsigned int timestamp;
        freenect_sync_get_video((void**)(&data), &timestamp, 0, FREENECT_VIDEO_RGB);
        cvSetData(image, data, 640*3);
        cvCvtColor(image, image, CV_RGB2BGR);
        cvShowImage("RGB", image);
      }
      freenect_sync_stop();       
      cvFree(&image);
}

int*** readCamera() {
	return NULL;
}

int*** readSilhouette() {
	return NULL;
}
