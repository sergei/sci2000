#ifndef MHU2NMEA_TRUEWINDCOMPUTER_H
#define MHU2NMEA_TRUEWINDCOMPUTER_H


class TrueWindComputer {
public:
    /**
     * Compute true wind from apparent wind
     * @param spd Boat speed in knots
     * @param aws Apparent wind speed in knots
     * @param awaRad Apparent wind angle in radians [0:2PI] (0 is straight ahead, PI is straight behind)
     * @param twaRad True wind angle in radians [0:2PI]
     * @param tws True wind speed in knots
     * @return True if computation was successful
     */
    static bool computeTrueWind(float spd, float aws, float awaRad, float &twaRad, float &tws);
};


#endif //MHU2NMEA_TRUEWINDCOMPUTER_H
