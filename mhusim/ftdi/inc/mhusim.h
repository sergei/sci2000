#ifndef FTDI_MHUSIM_H
#define FTDI_MHUSIM_H

#include <ftd2xx.h>

#ifdef __cplusplus
extern "C" {
#endif

int mhusim(FT_HANDLE ftHandle, double awsKts, double awaDeg);


#ifdef __cplusplus
}
#endif

#endif //FTDI_MHUSIM_H
