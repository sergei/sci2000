#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "ftd2xx.h"
#include "libft4222.h"

#ifndef _countof
    #define _countof(a) (sizeof((a))/sizeof(*(a)))
#endif

#define CHANNELS_NUM 4 // A, B, C, D
#define BYTES_PER_CHANNEL 2 // Each command has two bytes


static uint8 commandBuffer[CHANNELS_NUM * BYTES_PER_CHANNEL];

    
static void hexdump(uint8 *address, uint16 length)
{
    char      buf[3*8 + 2 + 8 + 1];
    char      subString[4];
    int       f;
    int       offsetInLine = 0;
    char      printable;
    char      unprinted = 0;

    buf[0] = '\0';
    
    for (f = 0; f < length; f++)
    {
        offsetInLine = f % 8;
        
        if (offsetInLine == 0)
        {
            // New line.  Display previous line...
            printf("%s\n%p: ", buf, address + f);
            unprinted = 0;
            // ...and clear buffer ready for the new line.
            memset(buf, (int)' ', sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
        }
        
        sprintf(subString, "%02x ", (unsigned int)address[f]);        
        memcpy(&buf[offsetInLine * 3], subString, 3);
        
        if ( isprint((int)address[f]) )
            printable = (char)address[f];
        else
            printable = '.';
        sprintf(subString, "%c", printable);
        memcpy(&buf[3*8 + 2 + offsetInLine], subString, 1);        
        
        unprinted++; // Remember 
    }
    
    if (unprinted)
        printf("%s\n", buf);
        
    printf("\n");
}



FT_HANDLE openI2CMaster(DWORD locationId)
{
    FT_STATUS            ftStatus;
    FT_HANDLE            ftHandle = (FT_HANDLE)NULL;
    FT4222_STATUS        ft4222Status;
    FT4222_Version       ft4222Version;

    ftStatus = FT_OpenEx((PVOID)(uintptr_t)locationId, 
                         FT_OPEN_BY_LOCATION, 
                         &ftHandle);
    if (ftStatus != FT_OK)
    {
        printf("FT_OpenEx failed (error %d)\n", 
               (int)ftStatus);
        goto error;
    }
    
    ft4222Status = FT4222_GetVersion(ftHandle,
                                     &ft4222Version);
    if (FT4222_OK != ft4222Status)
    {
        printf("FT4222_GetVersion failed (error %d)\n",
               (int)ft4222Status);
        goto error;
    }
    
    printf("Chip version: %08X, LibFT4222 version: %08X\n",
           (unsigned int)ft4222Version.chipVersion,
           (unsigned int)ft4222Version.dllVersion);

    // Configure the FT4222 as an I2C Master
    ft4222Status = FT4222_I2CMaster_Init(ftHandle, 400);
    if (FT4222_OK != ft4222Status)
    {
        printf("FT4222_I2CMaster_Init failed (error %d)!\n",
               ft4222Status);
        goto error;
    }

    return ftHandle;

error:
    if (ftHandle != (FT_HANDLE)NULL)
    {
        (void)FT4222_UnInitialize(ftHandle);
        (void)FT_Close(ftHandle);
    }

    return (FT_HANDLE)NULL;
}

void closeI2CMaster(FT_HANDLE ftHandle)
{
    if (ftHandle != (FT_HANDLE)NULL)
    {
        (void)FT4222_UnInitialize(ftHandle);
        (void)FT_Close(ftHandle);
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
int writeDac(FT_HANDLE ftHandle, uint16 dacA,  uint16 dacB,  uint16 dacC,  uint16 dacD )
{
    int                  success = 0;
    FT4222_STATUS        ft4222Status;
    const uint16         slaveAddr = 0x64;
    uint16               bytesToWrite;
    uint16               bytesWritten = 0;

    // Reset the I2CM registers to a known state.
    ft4222Status = FT4222_I2CMaster_Reset(ftHandle);
    if (FT4222_OK != ft4222Status)
    {
        printf("FT4222_I2CMaster_Reset failed (error %d)!\n",
               ft4222Status);
        goto exit;
    }

    uint16 dac = dacA;
    int idx = 0;
    commandBuffer[idx++] = 0x00U | (0x00FFU & (dac >> 8 ) );
    commandBuffer[idx++] = (uint8)(0x00FFU & dac);
    dac = dacB;
    commandBuffer[idx++] = 0x00U | (0x00FFU & (dac >> 8 ) );
    commandBuffer[idx++] = (uint8)(0x00FFU & dac);
    dac = dacC;
    commandBuffer[idx++] = 0x00U | (0x00FFU & (dac >> 8 ) );
    commandBuffer[idx++] = (uint8)(0x00FFU & dac);
    dac = dacD;
    commandBuffer[idx++] = 0x00U | (0x00FFU & (dac >> 8 ) );
    commandBuffer[idx++] = (uint8)(0x00FFU & dac);

    printf("Writing %d bytes...\n", idx);
    hexdump(commandBuffer, CHANNELS_NUM * BYTES_PER_CHANNEL);


    bytesToWrite = idx;
    ft4222Status = FT4222_I2CMaster_Write(ftHandle,
                                          slaveAddr,
                                          commandBuffer,
                                          bytesToWrite,
                                          &bytesWritten);
    if (FT4222_OK != ft4222Status)
    {
        printf("FT4222_I2CMaster_Write 2 failed (error %d)\n",
               (int)ft4222Status);
        goto exit;
    }

    if (bytesWritten != bytesToWrite)
    {
        printf("FT4222_I2CMaster_Write wrote %u of %u bytes.\n",
               bytesWritten,
               bytesToWrite);
        goto exit;
    }

    uint8 controllerStatus=0;
    ft4222Status = FT4222_I2CMaster_GetStatus(ftHandle, &controllerStatus);
    if (FT4222_OK != ft4222Status)
    {
        printf("FT4222_I2CMaster_GetStatus failed (error %d)\n",
               (int)ft4222Status);
        goto exit;
    }
    printf("controllerStatus %02X \n", controllerStatus);


    success = 1;

exit:

    return success;
}
#pragma clang diagnostic pop

int findFt422(DWORD *locId){
    FT_STATUS                 ftStatus;
    FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;
    DWORD                     numDevs = 0;
    int                       i;
    int                       retCode = 0;
    int                       found4222 = 0;

    ftStatus = FT_CreateDeviceInfoList(&numDevs);
    if (ftStatus != FT_OK)
    {
        printf("FT_CreateDeviceInfoList failed (error code %d)\n",
               (int)ftStatus);
        retCode = -10;
        goto exit;
    }

    if (numDevs == 0)
    {
        printf("No devices connected.\n");
        retCode = -20;
        goto exit;
    }

    /* Allocate storage */
    devInfo = calloc((size_t)numDevs,
                     sizeof(FT_DEVICE_LIST_INFO_NODE));
    if (devInfo == NULL)
    {
        printf("Allocation failure.\n");
        retCode = -30;
        goto exit;
    }

    /* Populate the list of info nodes */
    ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
    if (ftStatus != FT_OK)
    {
        printf("FT_GetDeviceInfoList failed (error code %d)\n",
               (int)ftStatus);
        retCode = -40;
        goto exit;
    }

    for (i = 0; i < (int)numDevs; i++)
    {
        unsigned int devType = devInfo[i].Type;
        size_t       descLen;

        if (devType == FT_DEVICE_4222H_0)
        {
            // In mode 0, the FT4222H presents two interfaces: A and B.
            descLen = strlen(devInfo[i].Description);

            if ('A' == devInfo[i].Description[descLen - 1])
            {
                // Interface A may be configured as an I2C master.
                printf("\nDevice %d is interface A of mode-0 FT4222H:\n",
                       i);
                printf("  0x%08x  %s  %s\n",
                       (unsigned int)devInfo[i].ID,
                       devInfo[i].SerialNumber,
                       devInfo[i].Description);
                *locId =  devInfo[i].LocId;
            }
            else
            {
                // Interface B of mode 0 is reserved for GPIO.
                printf("Skipping interface B of mode-0 FT4222H.\n");
            }

            found4222++;
        }

        if (devType == FT_DEVICE_4222H_1_2)
        {
            // In modes 1 and 2, the FT4222H presents four interfaces but
            // none is suitable for I2C.
            descLen = strlen(devInfo[i].Description);

            printf("Skipping interface %c of mode-1/2 FT4222H.\n",
                   devInfo[i].Description[descLen - 1]);

            found4222++;
        }

        if (devType == FT_DEVICE_4222H_3)
        {
            // In mode 3, the FT4222H presents a single interface.
            // It may be configured as an I2C Master.
            printf("\nDevice %d is mode-3 FT4222H (single Master/Slave):\n",
                   i);
            printf("  0x%08x  %s  %s\n",
                   (unsigned int)devInfo[i].ID,
                   devInfo[i].SerialNumber,
                   devInfo[i].Description);
            *locId =  devInfo[i].LocId;

            found4222++;
        }
    }

    if (!found4222)
        printf("No FT4222 found.\n");

    exit:
    free(devInfo);
    return retCode;

}

