#include "SensorFusion.h"

#include "esp_sensor_fusion.h"

void process_fusion_data(SensorFusionGlobals *sfg){

}

void SensorFusion::Start() {
    start_fusing(process_fusion_data);
}
