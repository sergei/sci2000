#include <cmath>
#include "MagDeviation.h"

// Use the approximation described in https://docs.google.com/spreadsheets/d/1NmPyGinMX7lg4pjzpNX2Fdu4xw0y0o2_KDigX1-a45U/edit#gid=326275003

float MagDeviation::correct(float hdgDeg) const {
    float deviaton = A0 - (float)(A1 * sin((hdgDeg - PHI1) * M_PI / 180.0f));
    return hdgDeg + deviaton;
}
