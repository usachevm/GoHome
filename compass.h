#ifndef __COMPASS_H__
#define __COMPASS_H__

#include "libbip.h"

#if (!defined bool)
typedef char bool;
#endif

void compassEnable();
void compassDisable();

bool isCalibrationRequired();

int getAzimuh();

#endif
