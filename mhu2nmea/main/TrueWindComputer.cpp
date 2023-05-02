#include <cmath>
#include "TrueWindComputer.h"

bool TrueWindComputer::computeTrueWind(float spd, float aws, float awaRad, float &twaRad, float &tws) {

    // Use information from http://en.wikipedia.org/wiki/Apparent_wind

    // Inputs
    double A = aws;
    double V = spd;
    double beta = awaRad;

    // True wind speed
    double W = A * A + V * V - 2 * A * V * cos( beta );

    // Do the sanity test. If true wind speed is less than one knot don't bother with the rest of the computations
    // and assume true wind angle and speed equals to the apparent ones
    double alpha = beta;
    if ( W > 1. )
    {
        W = sqrt( W );

        double r = ( A * cos( beta ) - V ) / W;
        // Another check for data sanity
        if ( r > 1. )
        {
            return false;
        }

        // Safe to take arc cosine
        alpha = acos( r );

        // Put TWA to the same tack as AWA
        if ( beta > M_PI )
            alpha = M_TWOPI - alpha;
    }
    else if ( W < 0. ) // Nonsense input data
    {
        return false;
    }

    // Since we made here all data makes sense now

    tws =  (float)W ;
    twaRad =  (float)alpha;
    return true;
}
