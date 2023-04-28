#ifndef MHU2NMEA_TRUEWINDCOMPUTER_H
#define MHU2NMEA_TRUEWINDCOMPUTER_H


class TrueWindComputer {
public:
    static bool computeTrueWind(float spd, float aws, float awaRad, float &twaRad, float &tws);
};


#endif //MHU2NMEA_TRUEWINDCOMPUTER_H
