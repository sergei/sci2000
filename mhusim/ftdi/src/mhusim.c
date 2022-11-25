#include <stdio.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include "mcp4728.h"
#include "mhusim.h"

#define PI 3.1415169535

static volatile int keepRunning = 1;

void intHandler() {
    keepRunning = 0;
}

static uint16 sin2dac(double v){
    return  (uint16)((v + 1.) * 0.5 * MAX_DAC_VALUE);
}


int mhusim(FT_HANDLE ftHandle, double awsKts, double awaDeg){

    printf("awaDeg=%.0f awsKts=%.1f\n", awaDeg, awsKts);
    printf("Press Ctrl-C to exit\n");

    signal(SIGINT, intHandler);


    double awaRad = awaDeg * PI / 180.;

    double twsPeriodSec = 1. /  awsKts;
    double twsPulseLenSec = twsPeriodSec * 0.3;


    double dtSec = 0.005;
    struct timespec ts = {
            (time_t) dtSec,
            (long) ((dtSec - (time_t) dtSec) * 1e9)
    };

    double t = 0;

    while(keepRunning){

        double a = sin(awaRad - PI / 2 );
        double b = sin(awaRad + PI - PI / 6 );
        double c = sin(awaRad + PI / 6 );

        double taws = fmod(t, twsPeriodSec);
        int16 dacD = taws < twsPulseLenSec ? MAX_DAC_VALUE: 0;

//        printf("t=%.3lf awaDeg=%.2f r=%.2f g=%.2f b=%.2f dacD=%03X\n", t, awaDeg, a, b, c, dacD);
        int rc = writeDac(ftHandle, sin2dac(a),  sin2dac(b),  sin2dac(c),  dacD );
        if ( rc )
            return rc;

        nanosleep(&ts, &ts);
        t += dtSec;
    }

    return 0;

}
