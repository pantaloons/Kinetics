#include "calibration.hpp"

#include <vector>
#include <cstdio>

#include <cvblob.h>

/**
 * See: http://opencv.willowgarage.com/wiki/VideoSurveillance
 * http://research.microsoft.com/en-us/um/people/chazhang/publications/3dpvt10_ChaZhang.pdf
 * http://mmlab.disi.unitn.it/wiki/index.php/Mixture_of_Gaussians_using_OpenCV
 */
 
#define TRAIN_FRAMES 256
#define DISCARD_FRAMES 8

uint8_t walls[GAME_HEIGHT][GAME_WIDTH] = {};

extern uint8_t colorBufs[3][GAME_HEIGHT][GAME_WIDTH][3];
extern int colorPos;

extern uint16_t depthBufs[2][GAME_HEIGHT][GAME_WIDTH];
extern int depthPos;

static bool reset;
static int training;
static cv::BackgroundSubtractorMOG2 bgModel;
static cv::Mat foreground;

void createModel() {
	foreground = cv::Mat(GAME_HEIGHT, GAME_WIDTH, CV_8UC1);
}

void updateModel() {
	if(reset) {
		training = DISCARD_FRAMES;
		bgModel = cv::BackgroundSubtractorMOG2();
		foreground = cv::Mat(GAME_HEIGHT, GAME_WIDTH, CV_8UC1);
		reset = false;
	}
	
	if(training < DISCARD_FRAMES) ;
	else if(training < TRAIN_FRAMES) {
		cv::Mat frame(GAME_HEIGHT, GAME_WIDTH, CV_8UC3, colorBufs[colorPos]);
		bgModel.operator()(frame, foreground);
	}
	else {
		if(training == TRAIN_FRAMES) {
			printf("Calibration done!\n");
		}
		cv::Mat frame(GAME_HEIGHT, GAME_WIDTH, CV_8UC3, colorBufs[colorPos]);
		bgModel.operator()(frame, foreground, 1e-8);
	}
	training++;
}

void resetModel() {
	reset = true;
}

void threshhold() {
	/*
	IplImage ipl = foreground;
	IplImage *labelImg = cvCreateImage(cvGetSize(&ipl), IPL_DEPTH_LABEL, 1);
	cvb::CvBlobs blobs;
	cvb::cvLabel(&ipl, labelImg, blobs);
	cvb::cvRenderBlobs(labelImg, blobs, &ipl, &ipl);
	
	foreground = cv::Mat(&ipl);*/
	for(int i = 0; i < GAME_HEIGHT; i++) {
		for(int j = 0; j < GAME_WIDTH; j++) {
			if(foreground.at<uint8_t>(i, j)) walls[i][j] = 255;
			else walls[i][j] = 0;
		}
	}
}
