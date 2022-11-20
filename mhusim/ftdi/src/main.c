#include <stdio.h>
#include <time.h>
#include <math.h>
#include "mcp4728.h"

#define PI 3.1415169535
#define MAX_DAC_VALUE  ((1U << 12U) - 1)


uint16 sin2dac(double v){
    return  (uint16)((v + 1.) * 0.5 * MAX_DAC_VALUE);
}

int main() {
    DWORD locationId;
    int retCode = findFt422(&locationId);
    if ( !retCode ){

        FT_HANDLE ftHandle = openI2CMaster( locationId );

        if ( ftHandle != (FT_HANDLE)NULL ){


            double twaPeriodSec = 1;
            double omegaTwa = 2. * PI / twaPeriodSec;

            double twsPeriodSec = 1;
            double twsPulseLenSec = 0.1;


            double dtSec = 0.01;
            struct timespec ts = {
                    (time_t) dtSec,
                    (long) ((dtSec - (time_t) dtSec) * 1e9)
            };

            double t = 0;

            for(int i =0 ; i < 10000; i++){

                double a = sin(omegaTwa * t );
                double b = sin(omegaTwa * t +  2 * PI / 3 );
                double c = sin(omegaTwa * t  + 4 * PI / 3 );

                double tTwa = fmod(t, twsPeriodSec);
                int16 dacD = tTwa < twsPulseLenSec ? MAX_DAC_VALUE: 0;


                printf("t=%.3lf a=%.2f b=%.2f c=%.2f dacD=%03X\n", t, a, b, c, dacD);
                writeDac(ftHandle, sin2dac(a),  sin2dac(b),  sin2dac(c),  dacD );
                nanosleep(&ts, &ts);
                t += dtSec;
            }

            closeI2CMaster(ftHandle);
        }

    }
    return 0;
}
