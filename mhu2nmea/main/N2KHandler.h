#ifndef MHU2NMEA_N2KHANDLER_H
#define MHU2NMEA_N2KHANDLER_H


class N2KHandler {
public:
    void Init();
    void Update(bool isValid, double awsKts, double awaDeg);
private:
    unsigned char uc_WindSeqId = 0;
};


#endif //MHU2NMEA_N2KHANDLER_H
