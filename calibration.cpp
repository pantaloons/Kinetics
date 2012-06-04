#include "calibration.hpp"

#include <vector>
#include <cstdio>

/**
 * See: http://opencv.willowgarage.com/wiki/VideoSurveillance
 * http://research.microsoft.com/en-us/um/people/chazhang/publications/3dpvt10_ChaZhang.pdf
 * http://mmlab.disi.unitn.it/wiki/index.php/Mixture_of_Gaussians_using_OpenCV
 */
 
/* Determined through trial and error, the camera self adjusts around frame 40 */
#define TRAIN_FRAMES 90
#define DISCARD_FRAMES 8

uint8_t walls[GAME_HEIGHT][GAME_WIDTH] = {};

extern uint8_t colorBufs[3][GAME_HEIGHT][GAME_WIDTH][3];
extern int colorPos;

extern uint16_t depthBufs[2][GAME_HEIGHT][GAME_WIDTH];
extern int depthPos;

static bool reset;
static int training;
static cv::BackgroundSubtractorMOG2 colorModel, depthModel;
static cv::Mat colorFG, depthFG;
static cv::Mat gray;

#ifdef DEBUG
uint8_t colorFGDebug[GAME_HEIGHT][GAME_WIDTH];
uint8_t depthFGDebug[GAME_HEIGHT][GAME_WIDTH];
uint8_t colorFGDebugFilt[GAME_HEIGHT][GAME_WIDTH];
uint8_t depthFGDebugFilt[GAME_HEIGHT][GAME_WIDTH];
uint8_t finalBefore[GAME_HEIGHT][GAME_WIDTH];
#endif

void createModel() {
	colorFG = cv::Mat(GAME_HEIGHT, GAME_WIDTH, CV_8UC1);
	depthFG = cv::Mat(GAME_HEIGHT, GAME_WIDTH, CV_8UC1);
}

void updateModel() {
	if(reset) {
		training = DISCARD_FRAMES;
		colorModel = cv::BackgroundSubtractorMOG2();
		depthModel = cv::BackgroundSubtractorMOG2();
		colorFG = cv::Mat(GAME_HEIGHT, GAME_WIDTH, CV_8UC1);
		depthFG = cv::Mat(GAME_HEIGHT, GAME_WIDTH, CV_8UC1);
		reset = false;
	}
	
	if(training < DISCARD_FRAMES) ;
	else if(training < TRAIN_FRAMES) {
		cv::Mat color(GAME_HEIGHT, GAME_WIDTH, CV_8UC3, colorBufs[colorPos]);
		cv::Mat depth(GAME_HEIGHT, GAME_WIDTH, CV_MAKETYPE(11, 1), depthBufs[depthPos]);
		colorModel.operator()(color, colorFG);
		depthModel.operator()(depth, depthFG);
	}
	else {
		if(training == TRAIN_FRAMES) printf("Calibration done!\n");
		cv::Mat color(GAME_HEIGHT, GAME_WIDTH, CV_8UC3, colorBufs[colorPos]);
		cv::Mat depth(GAME_HEIGHT, GAME_WIDTH, CV_MAKETYPE(11, 1), depthBufs[depthPos]);
		colorModel.operator()(color, colorFG, 1e-10);
		depthModel.operator()(depth, depthFG, 1e-10);
	}
	training++;
}

void resetModel() {
	reset = true;
}

void threshhold() {	
#ifdef DEBUG
	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			colorFGDebug[i][j] = colorFG.at<uint8_t>(i, j);
			depthFGDebug[i][j] = depthFG.at<uint8_t>(i, j);
		}
	}
#endif
	
	cv::medianBlur(colorFG, colorFG, 5);
	cv::medianBlur(depthFG, depthFG, 5);
	
#ifdef DEBUG
	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			colorFGDebugFilt[i][j] = colorFG.at<uint8_t>(i, j);
			depthFGDebugFilt[i][j] = depthFG.at<uint8_t>(i, j);
		}
	}
#endif
	
	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			if(colorFG.at<uint8_t>(i, j) && depthFG.at<uint8_t>(i, j)) walls[i][j] = 255;
			else walls[i][j] = 0;
		}
	}
	
#ifdef DEBUG
	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			finalBefore[i][j] = walls[i][j];
		}
	}
#endif

	cv::Mat wallMat(GAME_HEIGHT, GAME_WIDTH, CV_8UC1, walls);
	cv::medianBlur(wallMat, wallMat, 5);
	cv::dilate(wallMat, wallMat, cv::Mat(cv::Size(7, 7), CV_8UC1));
	cv::erode(wallMat, wallMat, cv::Mat(cv::Size(5, 5), CV_8UC1));
}
