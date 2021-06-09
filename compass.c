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

//!    определения номеров сенсоров для функции sensors_enable
#define SENSOR_TYPE_ACCEL        1
#define SENSOR_TYPE_COMPASS      3
#define SENSOR_TYPE_PRESSURE     7
#define SENSOR_TYPE_GPS          8

// Сборка под нужную прошивку
#define FW_11648

#ifdef FW_11648
#define GET_AZIMUTH				0x08038600
#define RESUME_COMPASS_TASK		0x08038FDC
#define SWITCH_SENSOR			0x08058D60
#define IS_CALIBRATION_REQUIRED	0x0803882C
#else 
#ifdef FW_11536
#define GET_AZIMUTH				0x0803906C
#define RESUME_COMPASS_TASK		0x08039A1C
#define SWITCH_SENSOR			0x08059360
#define IS_CALIBRATION_REQUIRED	0x08039298
#else 
#ifdef FW_11205
#define GET_AZIMUTH				0x08039AF4
#define RESUME_COMPASS_TASK		0x0803A404
#define SWITCH_SENSOR			0x0805993C
#define IS_CALIBRATION_REQUIRED	0x08039D1C
#endif
#endif
#endif

typedef int (*getAzimuth_FW) (void);

int getAzimuth() {
	volatile getAzimuth_FW fn = GET_AZIMUTH | 1;
	return fn();
}

typedef void (*checkAndAllocateCompassBuffers_FW) (void);

static void checkAndAllocateCompassBuffers() {
	volatile checkAndAllocateCompassBuffers_FW fn = RESUME_COMPASS_TASK | 1;
	fn();
}

typedef void (*switchSensor_FW) (int sensor_idx, int en);

void switchSensor(int sensor_idx, int en) {
	volatile switchSensor_FW fn = SWITCH_SENSOR | 1;
	fn(sensor_idx, en != 0);
}

typedef bool (*isCalibrationRequired_FW) ();

bool isCalibrationRequired() {
	volatile isCalibrationRequired_FW fn = IS_CALIBRATION_REQUIRED | 1;
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