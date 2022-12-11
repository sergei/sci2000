#ifndef MHU2NMEA_N2KHANDLER_H
#define MHU2NMEA_N2KHANDLER_H

#include "freertos/FreeRTOS.h"
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>


class N2KHandler {
public:
    N2KHandler();
    void StartTask();
    void SetAwa(bool  isValid, float value);
    void SetAws(bool  isValid, float value);

    [[noreturn]] void N2KTask();

private:
    void Init();

    unsigned char uc_WindSeqId = 0;

    SemaphoreHandle_t awaMutex;
    bool isAwaValid = false;
    float awaRad = 0;
    int64_t awaUpdateTime = 0;

    SemaphoreHandle_t awsMutex;
    bool isAwsValid = false;
    float awsKts = 0;
    int64_t awsUpdateTime = 0;


};


#endif //MHU2NMEA_N2KHANDLER_H
