#include <stdio.h>
#include "mcp4728.h"

int main() {
    DWORD locationId;
    int retCode = findFt422(&locationId);
    if ( !retCode ){

        FT_HANDLE ftHandle = openI2CMaster( locationId);
        writeDac(ftHandle, 1024,  512,  256,  2048 );
        writeDac(ftHandle, 3000,  1000,  500,  600 );
        closeI2CMaster(ftHandle);

    }
    return 0;
}
