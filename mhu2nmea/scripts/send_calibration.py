import argparse

import serial

from scripts.n2k_encoder import n2k_to_can_id, can_id_to_n2k

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

    msg_sent = False
    try:
        with serial.Serial(args.port) as ser:
            while True:
                line = ser.readline()

                if parse_line(line):
                    return

                if not msg_sent:
                    print(f'Sending [{raw_msg}]')
                    ser.write(raw_msg)
                    msg_sent = True

    except IOError as error:
        print(f'{error}')


def parse_mhu_cal(data_ascii):
    print(data_ascii)
    data = [int(x, 16) for x in data_ascii]
    print(data)
    dlc = data[0]
    awa_cal = (data[1]) | (data[2] << 8)
    aws_cal = (data[3]) | (data[4] << 8)
    print(f'len={dlc} awa_cal = 0x{awa_cal:04x} aws_cal=0x{aws_cal:04x}')
    awa_cal *= 0.001
    aws_cal *= 0.001
    print(f'awa_cal = {awa_cal:.1f} aws_cal={aws_cal:.3f}')
    return awa_cal, aws_cal


def parse_line(line):
    t = line.decode('ascii').split()
    print(t)
    can_id = int(t[2], 16)
    prio, pgn, src, dst = can_id_to_n2k(can_id)
    print(f'prio={prio}, pgn={pgn}, src={src}, dst={dst}')
    if pgn == GET_MHU_CALIBRATION_PGN:
        parse_mhu_cal(t[4:])
        return True
    return False


if __name__ == '__main__':
    cmdparser = argparse.ArgumentParser(description="Send calibartion to MHU to NMEA Unit using YDNU gateway")
    cmdparser.add_argument('--port',  help="Port name of YDNU", required=True)
    cmdparser.add_argument('--awa', type=float, help="AWA calibration (degrees)")
    cmdparser.add_argument('--aws', type=float, help="AWS calibration (percent)")

    calibrate(cmdparser.parse_args())
