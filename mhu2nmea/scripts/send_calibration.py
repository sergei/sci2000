import argparse

import serial

from scripts.n2k_encoder import n2k_to_can_id

ISO_REQUEST_PGN = 59904
SET_MHU_CALIBRATION_PGN = 130900
GET_MHU_CALIBRATION_PGN = 130901

MHU_CALIBR_SET_AWA = 0
MHU_CALIBR_SET_AWS = 1
MHU_CALIBR_CLEAR_AWA = 2
MHU_CALIBR_CLEAR_AWS = 3


class N2kMsg:
    def __init__(self, priority, pgn, src, dst):
        can_id = n2k_to_can_id(priority, pgn, src, dst)
        self.msg = f'{can_id:08x}'

    def add_byte(self, b):
        self.msg += f' {b:02X}'

    def add_val(self, val, size):
        for i in range(0, size*8, 8):
            b = (val >> i) & 0x00FF
            self.msg += f' {b:02X}'

    def get_msg(self):
        return (self.msg + '\r\n').encode('ascii')


def calibrate(args):
    if args.awa is not None:
        msg = N2kMsg(1, SET_MHU_CALIBRATION_PGN, 15, 255)
        msg.add_byte(MHU_CALIBR_SET_AWA)
        msg.add_val(int(args.awa * 10), 2)
    elif args.aws is not None:
        msg = N2kMsg(1, SET_MHU_CALIBRATION_PGN, 15, 255)
        msg.add_byte(MHU_CALIBR_SET_AWA)
        msg.add_val(int(args.aws * 10), 2)
    else:
        msg = N2kMsg(1, ISO_REQUEST_PGN, 15, 255)
        msg.add_val(GET_MHU_CALIBRATION_PGN, 3)

    raw_msg = msg.get_msg()

    print(f'Will send {raw_msg}')
    try:
        with serial.Serial(args.port) as ser:
            ser.write(raw_msg)
            while True:
                line = ser.readline()
                print(line)
    except IOError as error:
        print(f'{error}')


if __name__ == '__main__':
    cmdparser = argparse.ArgumentParser(description="Send calibartion to MHU to NMEA Unit using YDNU gateway")
    cmdparser.add_argument('--port',  help="Port name of YDNU", required=True)
    cmdparser.add_argument('--awa', type=float, help="AWA calibration (degrees)")
    cmdparser.add_argument('--aws', type=float, help="AWS calibration (percent)")

    calibrate(cmdparser.parse_args())
