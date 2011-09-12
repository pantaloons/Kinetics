#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

void initialize();

void swapPhysicsBuffers();
/**
 * Simulating delta milliseconds of physics, drawing the result into the paint buffer.
 */
unsigned long simulate(unsigned long delta, uint8_t *walls, uint8_t *rgb);
