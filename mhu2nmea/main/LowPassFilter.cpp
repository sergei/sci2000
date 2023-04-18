#include <cmath>
#include "LowPassFilter.h"

LowPassFilter::LowPassFilter(float cutoff_frequency) {
    // Set the cutoff frequency of the filter
    omega_cutoff = 2.0f * (float)M_PI * cutoff_frequency;

    // Set the filter time constant (T = 1/omega_cutoff)
    time_constant = 1.0f / omega_cutoff;

    // Set the initial filtered value
    filtered_value = 0.0;
}

float LowPassFilter::filterAngle(float value, float dt) {
    if ( ! initialized ) {
        // Set the initial filtered value
        filtered_value = value;
        initialized = true;
    }

    // Calculate smoothing factor alpha based on dt and time constant
    const float alpha = dt / (dt + time_constant);

    // Apply low pass filter formula to filter the input angle
    float delta = value - filtered_value;
    if (delta > M_PI) {
        delta -= M_TWOPI;
    }
    else if (delta < -M_PI) {
        delta += M_TWOPI;
    }
    filtered_value += alpha * delta;

    // Ensure output angle is in range 0 to 360 degrees
    while (filtered_value < 0) {
        filtered_value += M_TWOPI;
    }

    while (filtered_value >= 360.0) {
        filtered_value -= M_TWOPI;
    }

    return filtered_value;
}

float LowPassFilter::filter(float value, float dt) {
    if ( ! initialized ) {
        // Set the initial filtered value
        filtered_value = value;
        initialized = true;
    }

    // Calculate smoothing factor alpha based on dt and time constant
    const float alpha = dt / (dt + time_constant);

    // Apply low pass filter formula to filter the input angle
    float delta = value - filtered_value;
    filtered_value += alpha * delta;

    return filtered_value;
}
