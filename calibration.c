#include "calibration.h"

/**
 * See: http://opencv.willowgarage.com/wiki/VideoSurveillance
 */

uint8_t walls[GAME_HEIGHT][GAME_WIDTH] = {};

extern uint8_t colorBufs[3][GAME_HEIGHT][GAME_WIDTH][3];
extern int colorPos;

static IplImage *truth1, *truth2, *current, *diff;
static CvBGStatModel *bgModel;

void createModel() {
	truth1 = cvCreateImage(cvSize(GAME_WIDTH, GAME_HEIGHT), 8, 3);
	truth2 = cvCreateImage(cvSize(GAME_WIDTH, GAME_HEIGHT), 8, 3);
	current = cvCreateImage(cvSize(GAME_WIDTH, GAME_HEIGHT), 8, 3);
	diff = cvCreateImage(cvSize(GAME_WIDTH, GAME_HEIGHT), 8, 3);
	cvSetData(truth1, colorBufs[colorPos], GAME_WIDTH * 3);
	bgModel = cvCreateGaussianBGModel(truth1, NULL);
	updateModel();
}

void updateModel() {
	cvSetData(truth1, colorBufs[colorPos], GAME_WIDTH * 3);
	cvCopy(truth1, truth2, NULL);
}

void threshhold() {
	cvSetData(current, colorBufs[colorPos], GAME_WIDTH * 3);
	cvAbsDiff(truth2, current, diff);
	cvThreshold(diff, diff, 26, 0, CV_THRESH_TOZERO);
	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			if(diff->imageData[(i * GAME_WIDTH + j) * 3 + 0] ||
						diff->imageData[(i * GAME_WIDTH + j) * 3 + 1] ||
						diff->imageData[(i * GAME_WIDTH + j) * 3 + 1]) {	
				walls[i][j] = 255;
			}
			else walls[i][j] = 0;
		}
	}
}
