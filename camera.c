#include "camera.h"

freenect_context *context;
freenect_device *device;

#define HISTORY_SIZE 10

float t_gamma[2048];
uint16_t t_gamma_i[2048];

pthread_mutex_t rgbBufferMutex;
pthread_cond_t frameUpdateSignal = PTHREAD_COND_INITIALIZER;
int depthUpdate;
int rgbUpdate;

/* The buffer is managed (written to) by libfreenect
 * rgbStage is a staging area for new frames, only the latest frame is staged
 * the buffer and staging area get swapped on a frame update, while the front and staging
 * area get swappe on a render (front is the rendering buffer) */
uint8_t *rgbBuffer, *rgbStage, *rgbFront;
/*
 * As above, there is no back buffer because libfreenect uses an internal depth buffer
 */
uint16_t *depthStage, *depthFront;

/* 
 * Buffer for maximum depth for that pixel
 */
uint16_t *backgroundDepth;


/*
 * Some debugging buffers for depth imaging
 */
uint8_t *depthImageStage, *depthImageFront;

int initCamera() {
	/* Black magic... this converts kinect depth values to real distance */
	for (int i = 0; i < 2048; i++) { 
			const float k1 = 1.1863; 
			const float k2 = 2842.5; 
			const float k3 = 0.1236; 
			const float depth = k3 * tanf(i/k2 + k1); 
			t_gamma[i] = depth; 

			float v = i/2048.0;
			v = powf(v, 3) * 6;
			t_gamma_i[i] = v * 6 * 256;
	}
	
	if(freenect_init(&context, NULL) < 0) {
		printf("freenect_init() failed\n");
		return 1;
	}

	freenect_set_log_level(context, FREENECT_LOG_DEBUG);
	freenect_select_subdevices(context, (freenect_device_flags)(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));

	int devices = freenect_num_devices(context);
	printf("Number of devices found: %d\n", devices);
	if(devices < 1) return 1;

	if(freenect_open_device(context, &device, 0) < 0) {
		printf("Could not open device\n");
		return 1;
	}
	
	rgbBuffer = (uint8_t*)malloc(640 * 480 * 3 * sizeof(uint8_t));
	rgbStage = (uint8_t*)malloc(640 * 480 * 3 * sizeof(uint8_t));
	rgbFront = (uint8_t*)malloc(640 * 480 * 3 * sizeof(uint8_t));
	
	depthStage = (uint16_t*)malloc(640 * 480 * sizeof(uint16_t));
	depthFront = (uint16_t*)malloc(640 * 480 * sizeof(uint16_t));
	
	//~ backgroundDepth = (uint16_t*)malloc(640 * 480 * sizeof(uint16_t));
	// calloc inits all values to 0
	backgroundDepth = (uint16_t*)calloc(640 * 480, sizeof(uint16_t));
	
	depthImageStage = (uint8_t*)malloc(640 * 480 * 3 * sizeof(uint8_t));
	depthImageFront = (uint8_t*)malloc(640 * 480 * 3 * sizeof(uint8_t));

	return 0;
}

void swapRGBBuffers() {
	pthread_mutex_lock(&rgbBufferMutex);
	uint8_t *tmp;
	tmp = rgbFront;
	rgbFront = rgbStage;
	rgbStage = tmp;
	rgbUpdate = 0;
	pthread_mutex_unlock(&rgbBufferMutex);
}

void swapDepthBuffers() {
	pthread_mutex_lock(&rgbBufferMutex);
	uint16_t *tmp;
	tmp = depthFront;
	depthFront = depthStage;
	depthStage = tmp;
	depthUpdate = 0;
	pthread_mutex_unlock(&rgbBufferMutex);
}

void swapDepthImageBuffers() {
	pthread_mutex_lock(&rgbBufferMutex);
	uint8_t *tmp;
	tmp = depthImageFront;
	depthImageFront = depthImageStage;
	depthImageStage = tmp;
	pthread_mutex_unlock(&rgbBufferMutex);
}

void *cameraLoop(void *arg) {
	freenect_set_tilt_degs(device, 0);
	freenect_set_led(device, LED_RED);
	freenect_set_depth_callback(device, depthFunc);
	freenect_set_video_callback(device, rgbFunc);
	freenect_set_video_mode(device, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB));
	freenect_set_depth_mode(device, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));
	freenect_set_video_buffer(device, rgbBuffer);

	freenect_start_depth(device);
	freenect_start_video(device);
	
	int accelCount = 0;
	while(freenect_process_events(context) >= 0) {
		if (accelCount++ >= 2000)
		{
			accelCount = 0;
			freenect_raw_tilt_state* state;
			freenect_update_tilt_state(device);
			state = freenect_get_tilt_state(device);
			double dx, dy, dz;
			freenect_get_mks_accel(state, &dx, &dy, &dz);
			printf("\r raw acceleration: %4d %4d %4d  mks acceleration: %4f %4f %4f", state->accelerometer_x, state->accelerometer_y, state->accelerometer_z, dx, dy, dz);
			fflush(stdout);
		}
	}

	freenect_stop_depth(device);
	freenect_stop_video(device);
	freenect_close_device(device);
	freenect_shutdown(context);

	return NULL;
}

void rgbFunc(freenect_device *dev, void *rgb, uint32_t timestamp) {
	pthread_mutex_lock(&rgbBufferMutex);

	/* Swap buffers */
	assert(rgbBuffer == rgb);
	rgbBuffer = rgbStage;
	freenect_set_video_buffer(dev, rgbBuffer);
	rgbStage = rgb;

	rgbUpdate++;
	pthread_cond_signal(&frameUpdateSignal);
	pthread_mutex_unlock(&rgbBufferMutex);
}

void depthFunc(freenect_device *dev, void *v_depth, uint32_t timestamp) {
	uint16_t *depth = (uint16_t*)v_depth;

	pthread_mutex_lock(&rgbBufferMutex);
	for(int i = 0; i < 640 * 480; i++) {
		depthStage[i] = depth[i];
		int pval = t_gamma_i[depth[i]];
		
		if(pval > t_gamma[backgroundDepth[i]]) backgroundDepth[i] = pval;
		
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
	pthread_cond_signal(&frameUpdateSignal);
	pthread_mutex_unlock(&rgbBufferMutex);
}

IplImage *cvGetDepth()
{
	static IplImage *image = 0;
	if (!image) image = cvCreateImageHeader(cvSize(640,480), 16, 1);
	pthread_mutex_lock(&rgbBufferMutex);
	cvSetData(image, depthFront, 640*2);
	pthread_mutex_unlock(&rgbBufferMutex);
	return image;
}

IplImage *cvGetRGB()
{
	static IplImage *image = 0;
	if (!image) image = cvCreateImageHeader(cvSize(640,480), 8, 3);
	pthread_mutex_lock(&rgbBufferMutex);
	cvSetData(image, rgbFront, 640*3);
	pthread_mutex_unlock(&rgbBufferMutex);
	return image;
}
