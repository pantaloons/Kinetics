#include <stdio.h>
#include <pthread.h>
#include "libfreenect.h"
#include "camera.h"

freenect_context *context;
freenect_device *device;

/**
 * back: owned by libfreenect (implicit for depth)
 * mid: owned by callbacks, "latest frame ready"
 * front: owned by GL, "currently being drawn"
 */
uint8_t *depthMid, *depthFront;
uint8_t *rgbBack, *rgbMid, *rgbFront;

uint16_t t_gamma[2048];

extern pthread_mutex_t gl_backbuf_mutex;
extern pthread_cond_t gl_frame_cond;
extern int depthUpdate;
extern int rgbUpdate;

int initCamera() {
	for(int i = 0; i < 2048; i++) {
		float v = i / 2048.0f;
		v = powf(v, 3) * 6;
		t_gamma[i] = v * 6 * 256;
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
	
	depthMid = (uint8_t*)malloc(640 * 480 * 3);
	depthFront = (uint8_t*)malloc(640 * 480 * 3);
	
	rgbBack = (uint8_t*)malloc(640 * 480 * 3);
	rgbMid = (uint8_t*)malloc(640 * 480 * 3);
	rgbFront = (uint8_t*)malloc(640 * 480 * 3);

	return 0;
}

void swapRgbBuffers() {
	uint8_t *tmp;
	tmp = rgbFront;
	rgbFront = rgbMid;
	rgbMid = tmp;
	rgbUpdate = 0;
}

void swapDepthBuffers() {
	uint8_t *tmp;
	tmp = depthFront;
	depthFront = depthMid;
	depthMid = tmp;
	depthUpdate = 0;
}

void *cameraLoop(void *arg) {
	freenect_set_tilt_degs(device, 0);
	freenect_set_led(device, LED_RED);
	freenect_set_depth_callback(device, depthFunc);
	freenect_set_video_callback(device, rgbFunc);
	freenect_set_video_mode(device, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB));
	freenect_set_depth_mode(device, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));
	freenect_set_video_buffer(device, rgbBack);

	freenect_start_depth(device);
	freenect_start_video(device);
	
	int accelCount = 0;
	while(freenect_process_events(context) >= 0) {
		if (accelCount++ >= 1) //2000)
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

	// swap buffers
	assert(rgbBack == rgb);
	rgbBack = rgbMid;
	freenect_set_video_buffer(dev, rgbBack);
	rgbMid = (uint8_t*)rgb;

	rgbUpdate++;
	pthread_cond_signal(&gl_frame_cond);
	pthread_mutex_unlock(&gl_backbuf_mutex);
}

void depthFunc(freenect_device *dev, void *v_depth, uint32_t timestamp) {
	uint16_t *depth = (uint16_t*)v_depth;

	pthread_mutex_lock(&gl_backbuf_mutex);
	for(int i = 0; i < 640 * 480; i++) {
		int pval = t_gamma[depth[i]];
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
	}
	
	depthUpdate++;
	pthread_cond_signal(&gl_frame_cond);
	pthread_mutex_unlock(&gl_backbuf_mutex);
}

int *readCamera() {
	return NULL;
}

int *readSilhouette() {
	return NULL;
}
