#include "calibration.h"

/**
 * See: http://opencv.willowgarage.com/wiki/VideoSurveillance
 * http://research.microsoft.com/en-us/um/people/chazhang/publications/3dpvt10_ChaZhang.pdf
 */

uint8_t walls[GAME_HEIGHT][GAME_WIDTH] = {};

extern uint8_t colorBufs[3][GAME_HEIGHT][GAME_WIDTH][3];
extern int colorPos;

extern uint16_t depthBufs[2][GAME_HEIGHT][GAME_WIDTH];
extern int depthPos;

static int training;
static IplImage *colorImg, *modelImg;
static CvBGStatModel *bgModel;

void createModel() {
	colorImg = cvCreateImage(cvSize(GAME_WIDTH, GAME_HEIGHT), 8, 3);
	modelImg = cvCreateImage(cvSize(GAME_WIDTH, GAME_HEIGHT), 8, 1);	
}

void updateModel() {
	if(training < TRAIN_FRAMES) {
		if(training < DISCARD_FRAMES) ;
		else if(training == DISCARD_FRAMES) {
			cvSetData(colorImg, colorBufs[colorPos], GAME_WIDTH * 3);
			CvFGDStatModelParams params = {CV_BGFG_FGD_LC, CV_BGFG_FGD_N1C, CV_BGFG_FGD_N2C, CV_BGFG_FGD_LCC,
					CV_BGFG_FGD_N1CC, CV_BGFG_FGD_N2CC, 1, 1, 2*CV_BGFG_FGD_ALPHA_1, 3*CV_BGFG_FGD_ALPHA_2,
					2*CV_BGFG_FGD_ALPHA_3, CV_BGFG_FGD_DELTA, CV_BGFG_FGD_T, CV_BGFG_FGD_MINAREA};
			bgModel = cvCreateGaussianBGModel(colorImg, params);
		}
		else {
			cvSetData(colorImg, colorBufs[colorPos], GAME_WIDTH * 3);
			cvUpdateBGStatModel(colorImg, bgModel);
		}
		training++;
	}
	else {
		cvSetData(colorImg, colorBufs[colorPos], GAME_WIDTH * 3);
		cvUpdateBGStatModel(colorImg, bgModel, 0);
		cvCopy(bgMidel->foreground, modelImg);
		IplConvKernel *circle = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_ELLIPSE);        
		cvMorphologyEx(modelImg, modelImg, 0, circElem, CV_MOP_CLOSE, 3);
		cvReleaseStructuringElement(&circElem);
	}
}

void threshhold() {
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