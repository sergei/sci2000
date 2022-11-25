#include <stdio.h>
#include <time.h>
#include <math.h>
#include <signal.h>

#include "CLI11.hpp"

#include "mhusim.h"
#include "mcp4728.h"


int main(int argc, char **argv) {

    // Parse the arguments
    CLI::App app("mhusim");
    uint16_t aws=10;
    app.add_option("-s,--aws", aws, "Apparent Wind Speed (kts)")->required(true);
    uint16_t awa=330;
    app.add_option("-a,--awa", awa, "Apparent Wind Angle (degrees)")->required(true);
    try {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e) {
        printf("exiting %s", e.what());
        return app.exit(e);
    }

    DWORD locationId;
    int retCode = findFt422(&locationId);

    if ( !retCode ){

        FT_HANDLE ftHandle = openI2CMaster( locationId );

        if ( ftHandle != (FT_HANDLE)NULL ){
            double awsKts = aws;
            double awaDeg = awa;
            mhusim( ftHandle,  awsKts,  awaDeg);

        }

        printf("Exiting ...\n");

        closeI2CMaster(ftHandle);

    }
    return 0;
}
