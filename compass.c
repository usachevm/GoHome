#include "compass.h"

#ifdef BIPOS_COMPASS_SUPPORT

int getAzimuh() {
	return compass_get_degree();
}

bool isCalibrationRequired() {
	return compass_need_calibration()();
}

void compassEnable() {
	compass_task_resume();
	sensors_enable(SENSOR_TYPE_COMPASS, 1);
}

void compassDisable() {
	sensors_enable(SENSOR_TYPE_COMPASS, 0);
}

#else

// !! Firmware 1.1.6.48 ONLY !!

//!    определения номеров сенсоров для функции sensors_enable
#define SENSOR_TYPE_ACCEL        1
#define SENSOR_TYPE_COMPASS      3
#define SENSOR_TYPE_PRESSURE     7
#define SENSOR_TYPE_GPS          8

typedef int (*getAzimuh_FW) (void);

int getAzimuh() {
	volatile getAzimuh_FW fn = 0x08038600 | 1;
	return fn();
}

typedef void (*checkAndAllocateCompassBuffers_FW) (void);

static void checkAndAllocateCompassBuffers() {
	volatile checkAndAllocateCompassBuffers_FW fn = 0x08038FDC | 1;
	fn();
}

typedef void (*switchSensor_FW) (int sensor_idx, int en);

void switchSensor(int sensor_idx, int en) {
	volatile switchSensor_FW fn = 0x08058D60 | 1;
	fn(sensor_idx, en != 0);
}

typedef bool (*isCalibrationRequired_FW) ();

bool isCalibrationRequired() {
	volatile isCalibrationRequired_FW fn = 0x0803882C | 1;
	return fn();
}

void compassEnable() {
	checkAndAllocateCompassBuffers();
	switchSensor(SENSOR_TYPE_COMPASS, 1);
}

void compassDisable() {
	switchSensor(SENSOR_TYPE_COMPASS, 0);
}

#endif