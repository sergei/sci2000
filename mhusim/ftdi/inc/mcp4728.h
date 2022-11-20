#ifndef FTDI_MCP4728_H
#define FTDI_MCP4728_H

#include "ftd2xx.h"
#include "libft4222.h"


// Find if the FT4222 device is connected
int findFt422(DWORD *locationId);

// Open I2C Master
FT_HANDLE openI2CMaster(DWORD locationId);

// Close I2C Master
void closeI2CMaster(FT_HANDLE ftHandle);

// Write DAC values
int writeDac(FT_HANDLE ftHandle, uint16 dacA,  uint16 dacB,  uint16 dacC,  uint16 dacD );


#endif //FTDI_MCP4728_H
