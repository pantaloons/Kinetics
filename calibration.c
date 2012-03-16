#include "calibration.h"

/**
 * See: http://opencv.willowgarage.com/wiki/VideoSurveillance
 * http://opencv.willowgarage.com/wiki/VideoSurveillance
 */

uint8_t walls[GAME_WIDTH][GAME_HEIGHT] = {};

extern uint8_t colorBufs[3][GAME_WIDTH][GAME_HEIGHT][3];
extern int colorPos;

static uint8_t truth[GAME_WIDTH][GAME_HEIGHT][3] = {}; 

void updateModel() {
	memcpy(truth, colorBufs[colorPos], sizeof truth);
	/*static IplImage *color = 0, *depth = 0;
	if (!color) {
		color = cvCreateImageHeader(cvSize(GAME_WIDTH, GAME_HEIGHT), 8, 3);
		depth = cvCreateImageHeader(cvSize(GAME_WIDTH, GAME_HEIGHT), 8, 3);
	}
	cvSetData(image, color, GAME_WIDTH * 3);
	cvSetData(image, depth, GAME_WIDTH * 3);
	cvCopy(rgb, result, NULL);*/
}

void threshhold() {

}
