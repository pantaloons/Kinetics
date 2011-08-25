#include "camera.h"

freenect_context *context;
freenect_device *device;

#define HISTORY_SIZE 10

float t_gamma[2048];

extern pthread_mutex_t gl_backbuf_mutex;
extern pthread_cond_t gl_frame_cond;
extern int depthUpdate;
extern int rgbUpdate;

/* The buffer is managed (written to) by libfreenect
 * rgbStage is a staging area for new frames, only the latest frame is staged
 * the buffer and staging area get swapped on a frame update, while the front and staging
 * area get swappe on a render (front is the rendering buffer) */
uint8_t *rgbBuffer, *rgbStage, *rgbFront;
/*
 * As above, there is no back buffer because libfreenect uses an internal depth buffer
 */
float *depthStage, *depthFront;
/* These are the history frames, they get cycled every time the staging frame gets updated
 * frames[0] is the most recent frame. */
uint8_t **rgbFrames;
float **depthFrames;

int initCamera() {
	/* Black magic... this converts kinect depth values to real distance */
	for (int i = 0; i < 2048; i++) 
	{ 
			const float k1 = 1.1863; 
			const float k2 = 2842.5; 
			const float k3 = 0.1236; 
			const float depth = k3 * tanf(i/k2 + k1); 
			t_gamma[i] = depth; 
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
	
	
	depthStage = (float*)malloc(640 * 480 * sizeof(float));
	depthFront = (float*)malloc(640 * 480 * sizeof(float));
	
	rgbFrames = (uint8_t**)malloc(HISTORY_SIZE * sizeof(uint8_t*));
	depthFrames = (float**)malloc(HISTORY_SIZE * sizeof(float*));
	for(int i = 0; i < HISTORY_SIZE; i++) {
		rgbFrames[i] = (uint8_t*)malloc(HISTORY_SIZE * 640 * 480 * 3 * sizeof(uint8_t));
		depthFrames[i] = (float*)malloc(HISTORY_SIZE * 640 * 480 * sizeof(float));
	}

	return 0;
}

void swapRGBBuffers() {
	uint8_t *tmp;
	tmp = rgbFront;
	rgbFront = rgbStage;
	rgbStage = tmp;
	rgbUpdate = 0;
}

void swapDepthBuffers() {
	float *tmp;
	tmp = depthFront;
	depthFront = depthStage;
	depthStage = tmp;
	depthUpdate = 0;
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
	pthread_mutex_lock(&gl_backbuf_mutex);

	/* Swap buffers */
	assert(rgbBuffer == rgb);
	rgbBuffer = rgbStage;
	freenect_set_video_buffer(dev, rgbBuffer);
	rgbStage = rgb;
	for(int i = HISTORY_SIZE - 1; i > 0; i--) rgbFrames[i] = rgbFrames[i - 1];
	memcpy(rgbFrames[0], rgb, 640 * 480 * 3 * sizeof(uint8_t));

	rgbUpdate++;
	pthread_cond_signal(&gl_frame_cond);
	pthread_mutex_unlock(&gl_backbuf_mutex);
}

void depthFunc(freenect_device *dev, void *v_depth, uint32_t timestamp) {
	uint16_t *depth = (uint16_t*)v_depth;

	pthread_mutex_lock(&gl_backbuf_mutex);
	for(int i = HISTORY_SIZE - 1; i > 0; i--) depthFrames[i] = depthFrames[i - 1];
	for(int i = 0; i < 640 * 480; i++) {
		depthStage[i] = t_gamma[depth[i]];
		depthFrames[0][i] = t_gamma[depth[i]];
	}
	depthUpdate++;
	pthread_cond_signal(&gl_frame_cond);
	pthread_mutex_unlock(&gl_backbuf_mutex);
		/*
		int lb = pval & 0xff;
		switch(pval >> 8) {
			case 0:
				depthMid[3*i+0] = 255;
				depthMid[3*i+1] = 255 - lb;
				depthMid[3*i+2] = 255 - lb;
				break;
			case 1:
				depthMid[3*i+0] = 255;
				depthMid[3*i+1] = lb;
				depthMid[3*i+2] = 0;
				break;
			case 2:
				depthMid[3*i+0] = 255 - lb;
				depthMid[3*i+1] = 255;
				depthMid[3*i+2] = 0;
				break;
			case 3:
				depthMid[3*i+0] = 0;
				depthMid[3*i+1] = 255;
				depthMid[3*i+2] = lb;
				break;
			case 4:
				depthMid[3*i+0] = 0;
				depthMid[3*i+1] = 255 - lb;
				depthMid[3*i+2] = 255;
				break;
			case 5:
				depthMid[3*i+0] = 0;
				depthMid[3*i+1] = 0;
				depthMid[3*i+2] = 255 - lb;
				break;
			default:
				depthMid[3*i+0] = 0;
				depthMid[3*i+1] = 0;
				depthMid[3*i+2] = 0;
				break;
		}
		*/
}

int *readCamera() {
	return NULL;
}

int *readSilhouette() {
	return NULL;
}
