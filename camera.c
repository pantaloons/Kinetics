#include "camera.h"

static freenect_context *context;
static freenect_device *device;

static uint16_t gamma[2048];

pthread_mutex_t kinectMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t kinectSignal = PTHREAD_COND_INITIALIZER;

/* Three color buffers. One that is available (the front), one that is fetched,
 * and one that is written to by the kinect. We swap fetched and kinect buffers
 * when kinect posts a new frame, and front and fetch buffers when user requests
 * a swap. This keeps fresh frames available for the user but will drop unused ones
 * too. ColorPos[0..2] is the position of the front, fetch, and back frames within
 * the color buffers.
 */
uint_fast8_t colorBufs[3][GAME_WIDTH][GAME_HEIGHT][3];
static int colorPosR[3] = {0, 1, 2};
int colorPos = 0;
int colorUpdate = 0;

/* Two depth buffers. One that is available (the front), one that is swapped out.
 * depthPos is the position of the front within the depth buffers. */
uint_fast8_t depthBufs[2][GAME_WIDTH][GAME_HEIGHT][3];
int depthPos = 0;
int depthUpdate = 0;

bool initCamera() {
	/* Black magic... this converts kinect depth values to real distance */
	for (int i = 0; i < 2048; i++) { 
			const float k1 = 1.1863; 
			const float k2 = 2842.5; 
			const float k3 = 0.1236; 
			const float depth = k3 * tanf(i/k2 + k1); 

			float v = i/2048.0;
			v = powf(v, 3) * 6;
			gamma[i] = v * 6 * 256;
	}
	
	if(freenect_init(&context, NULL) < 0) {
		printf("freenect_init failed.\n");
		return false;
	}

	freenect_set_log_level(context, FREENECT_LOG_DEBUG);
	freenect_select_subdevices(context, (freenect_device_flags)(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));

	int devices = freenect_num_devices(context);
	printf("Number of devices found: %d\n", devices);
	if(devices < 1) return false;

	if(freenect_open_device(context, &device, 0) < 0) {
		printf("Could not open device\n");
		return false;
	}
	
	return true;
}

void swapColorBuffers() {
	pthread_mutex_lock(&kinectMutex);
	int tmp = colorPosR[0];
	colorPosR[0] = colorPosR[1];
	colorPosR[1] = tmp;
	colorPos = colorPosR[0];
	colorUpdate = 0;
	pthread_mutex_unlock(&kinectMutex);
}

void swapDepthBuffers() {
	pthread_mutex_lock(&kinectMutex);
	depthPos = (depthPos + 1) % 2;
	depthUpdate = 0;
	pthread_mutex_unlock(&kinectMutex);
}

void *cameraLoop(void *arg) {
	freenect_set_tilt_degs(device, 0);
	freenect_set_led(device, LED_RED);
	freenect_set_depth_callback(device, depthFunc);
	freenect_set_video_callback(device, rgbFunc);
	freenect_set_video_mode(device, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB));
	freenect_set_depth_mode(device, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));
	freenect_set_video_buffer(device, rgbBuffer);
	freenect_set_log_level(context, FREENECT_LOG_WARNING);

	freenect_start_depth(device);
	freenect_start_video(device);
	
	while(freenect_process_events(context) >= 0) ;

	freenect_stop_depth(device);
	freenect_stop_video(device);
	freenect_close_device(device);
	freenect_shutdown(context);

	return NULL;
}

static void colorFunc(freenect_device *dev, void *rgb, uint32_t timestamp) {
	pthread_mutex_lock(&kinectMutex);

	/* Swap back buffers */
	int tmp = colorPos[2];
	colorPosR[2] = colorPosR[1];
	freenect_set_video_buffer(dev, &colorBufs[colorPosR[2]]);
	colorPosR[1] = tmp;
	
	colorUpdate++;
	pthread_cond_signal(&kinectSignal);
	pthread_mutex_unlock(&kinectMutex);
}

static void depthFunc(freenect_device *dev, void *v_depth, uint32_t timestamp) {
	uint16_t *depth = (uint16_t*)v_depth;

	pthread_mutex_lock(&kinectMutex);
	for(int i = 0; i < GAME_WIDTH * GAME_HEIGHT; i++) {
		depthStage[i] = depth[i];
		int pval = t_gamma_i[depth[i]];
		
		int lb = pval & 0xff;
		switch (pval >> 8) {
			case 0:
				depthImageStage[3*i+0] = 255;
				depthImageStage[3*i+1] = 255 - lb;
				depthImageStage[3*i+2] = 255 - lb;
				break;
			case 1:
				depthImageStage[3*i+0] = 255;
				depthImageStage[3*i+1] = lb;
				depthImageStage[3*i+2] = 0;
				break;
			case 2:
				depthImageStage[3*i+0] = 255 - lb;
				depthImageStage[3*i+1] = 255;
				depthImageStage[3*i+2] = 0;
				break;
			case 3:
				depthImageStage[3*i+0] = 0;
				depthImageStage[3*i+1] = 255;
				depthImageStage[3*i+2] = lb;
				break;
			case 4:
				depthImageStage[3*i+0] = 0;
				depthImageStage[3*i+1] = 255 - lb;
				depthImageStage[3*i+2] = 255;
				break;
			case 5:
				depthImageStage[3*i+0] = 0;
				depthImageStage[3*i+1] = 0;
				depthImageStage[3*i+2] = 255 - lb;
				break;
			default:
				depthImageStage[3*i+0] = 0;
				depthImageStage[3*i+1] = 0;
				depthImageStage[3*i+2] = 0;
				break;
		}
	}
	depthUpdate++;
	pthread_cond_signal(&kinectSignal);
	pthread_mutex_unlock(&kinectMutex);
}
