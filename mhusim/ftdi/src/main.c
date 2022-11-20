#include <stdio.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include "mhusim.h"
#include "mcp4728.h"


int main() {
    DWORD locationId;

    int retCode = findFt422(&locationId);

    if ( !retCode ){

        FT_HANDLE ftHandle = openI2CMaster( locationId );

        if ( ftHandle != (FT_HANDLE)NULL ){
            double awsKts = 10;
            double awaDeg = 330;
            mhusim( ftHandle,  awsKts,  awaDeg);

        }

        printf("Exiting ...\n");

        closeI2CMaster(ftHandle);

    }
    return 0;
}
