import argparse

import serial

from scripts.n2k_encoder import n2k_to_can_id

SET_MHU_CALIBRATION_PGN = 130900


def calibrate(args):
    raw_msg = None
    can_id = n2k_to_can_id(1, SET_MHU_CALIBRATION_PGN, 15, 255)
    if args.awa is not None:
        value = int(args.awa * 10)
        raw_msg = f'{can_id:08x} 00 {(value << 8) & 0x00FF:02x} {value & 0x00FF:02x}\r\n'

    if raw_msg is not None:
        with serial.Serial(args.port) as ser:
            ser.write(raw_msg.encode('ascii'))
            print(raw_msg)


if __name__ == '__main__':
    cmdparser = argparse.ArgumentParser(description="Send calibartion to MHU to NMEA Unit using YDNU gateway")
    cmdparser.add_argument('--port',  help="Port name of YDNU", required=True)
    cmdparser.add_argument('--awa', type=float, help="AWA calibration (degrees)")
    cmdparser.add_argument('--aws', type=float, help="AWS calibration (percent)")

    calibrate(cmdparser.parse_args())
