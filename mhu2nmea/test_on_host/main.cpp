#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "../main/AWAComputer.h"

std::vector<std::string> splitCsvString(const std::string &line) {
    std::istringstream iss(line);
    std::string token;
    std::__1::vector<std::string> tokens;
    while (std::getline(iss, token, ',')){
        tokens.push_back(token);
    }
    return tokens;
}

void testAwaComputerOnLog(char *logFileName) {
    std::cout << "Reading log file " << logFileName << std::endl;

    std::ifstream logFileStream (logFileName, std::ios::in);

    std::string line;
    bool got_adc = false;

    std::vector<int16_t> adc_r;
    std::vector<int16_t> adc_g;
    std::vector<int16_t> adc_b;

    std::vector<float> raw_a;
    std::vector<float> est_a;
    std::vector<float> fw_r;
    std::vector<float> fw_g;
    std::vector<float> fw_b;
    std::vector<float> fw_raw_awa;
    std::vector<float> fw_est_awa;


    // Read log file

    while (std::getline(logFileStream, line)) {
        // Check if AWA_ADC string is in the line
        if (line.find("AWA_ADC") != std::string::npos) {
            std::vector<std::string> tokens = splitCsvString(line);
            if ( tokens.size() < 7 ){
                continue;
            }
            adc_r.push_back((int16_t)std::stoi(tokens[2]));
            adc_g.push_back((int16_t)std::stoi(tokens[4]));
            adc_b.push_back((int16_t)std::stoi(tokens[6]));
            got_adc = true;
        }

        if (got_adc && line.find("AWA,dt_sec,") != std::string::npos) {
            std::vector<std::string> tokens = splitCsvString(line);
            if ( tokens.size() < 17 ){
                continue;
            }
            raw_a.push_back(std::stof(tokens[4]));
            est_a.push_back(std::stof(tokens[6]));
            fw_r.push_back(std::stof(tokens[8]));
            fw_g.push_back(std::stof(tokens[10]));
            fw_b.push_back(std::stof(tokens[12]));
            fw_raw_awa.push_back(std::stof(tokens[14]));
            fw_est_awa.push_back(std::stof(tokens[16]));

            got_adc = false;
        }
    }

    // Process ADC values
    AWAComputer awaComputer;
    // Print CSV header
    std::cout << "AWA new" << "," << "AWA FW raw" << "," << "FW AWA filtered" << std::endl;
    for(int i = 0; i < adc_r.size(); i++ ) {
        float awa = awaComputer.computeAwa(adc_r[i], adc_g[i], adc_b[i], 0.1);

        float awa_deg = awa * 180.f / float(M_PI);

        // Print CSV line
        std::cout << awa_deg << "," << fw_raw_awa[i] << "," << fw_est_awa[i] << std::endl;
    }


}

bool testAwaComputerOnSim() {

    srand(1000);

    for( int n = 0; n < 100; n++){
        for( int i = 0; i < 360; i++ ){
            auto awa_sim = float(i * M_PI / 180.f);

            float red_u = - std::cos(awa_sim);
            float green_u = - std::sin(awa_sim - float(M_PI) / 6.f);
            float blue_u = std::sin(awa_sim + float(M_PI) / 6.f);

            float a = 1000.f;
            float noise = a * 0.05f * float(rand()) / float(RAND_MAX);

            auto red  = int16_t(a * ( red_u + 1) + noise);
            auto green = int16_t(a * ( green_u + 1) + noise);
            auto blue = int16_t(a * ( blue_u + 1) + noise);

            AWAComputer awaComputer;
            float awa_est = awaComputer.computeAwa(red, green, blue, 0.1);

            // Convert to degrees
            awa_sim = awa_sim * 180.f / float(M_PI);
            awa_est = awa_est * 180.f / float(M_PI);
            // Print CSV line
//            std::cout << awa_sim << "," << awa_est << std::endl;

            auto delta = std::abs(awa_sim - awa_est);
            if( delta > 180.f ){
                delta = 360.f - delta;
            }
            if (delta > 2 ){
                std::cout << "Error: awa_sim = " << awa_sim << ", awa_est = " << awa_est << std::endl;
                return false;
            }

        }
    }

    return true;
}

int main(int argc, char **argv) {


    if( ! testAwaComputerOnSim() ){
        return 1;
    }

    if( argc > 1){
        testAwaComputerOnLog(argv[1]);
    }

    return 0;
}
