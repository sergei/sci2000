#!/usr/bin/env python3

import argparse
import math
import serial

from n2k_encoder import n2k_to_can_id, can_id_to_n2k

AWS_SCALE = 1000
AWA_SCALE = 1000
SOW_SCALE = 1000

GROUP_PGN = 126208

MHU_CALIBRATION_PGN = 130900
SPEED_CALIBRATION_PGN = 130901

SCI_MFG_CODE = 2020  # Our mfg code.
SCI_INDUSTRY_CODE = 4  # Marine industry
BCAST_DST = 255
SRC = 0
PRIORITY = 2


class N2kMsg:
    def __init__(self, priority, pgn, src, dst):
        self.priority = priority
        self.pgn = pgn
        self.src = src
        self.dst = dst
        self.data = []

    def set_src(self, src):
        self.src = src

    def add_byte(self, b):
        self.data += [f'{b:02X}']

    def add_val(self, val, bytes_num):
        for i in range(0, bytes_num*8, 8):
            b = (val >> i) & 0x00FF
            self.data += [f'{b:02X}']

    def get_msgs(self):
        can_id = n2k_to_can_id(self.priority, self.pgn, self.src, self.dst)
        hdr = f'{can_id:08X}'

        # Check if it's single frame message or not
        dl = len(self.data)
        if dl <= 7:
            msg = (hdr + f' {dl:02X}' + ' '.join(self.data) + '\r\n').encode('ascii')
            return [msg]
        else:
            msgs = []
            payload = [f'{dl:02X}'] + self.data
            n_pad = (7 - len(payload) % 7) % 7
            payload += ['FF' for _ in range(n_pad)]
            frame_no = 0x40
            for i in range(0, len(payload), 7):
                chunk = ' '.join(payload[i:i+7])
                msg = (hdr + f' {frame_no:02X} ' + chunk + '\r\n').encode('ascii')
                msgs += [msg]
                frame_no += 1
            return msgs


class GroupFunction(N2kMsg):
    def __init__(self, grp_pgn, priority, src, dst):
        N2kMsg.__init__(self, priority, GROUP_PGN, src, dst)
        self.grp_pgn = grp_pgn

    def make_request(self):
        self.add_byte(0)                    # Field 1: Request Group Function Code = 0 (Request Message), 8 bits
        self.add_val(self.grp_pgn, 3)       # Field 2: PGN = 130900, 24 bits
        self.add_val(0xFFFFFFFF, 4)         # Field 3: Transmission interval = FFFF FFFF (Do not change interval), 32 bits
        self.add_val(0xFFFF, 2)             # Field 4: Transmission interval offset = 0xFFFF (Do not change offset), 16 bits
        self.add_byte(2)                    # Field 5: Number of Pairs of Request Parameters to follow = 2, 8 bits
        self.add_byte(1)                    # Field 6: Field number of requested parameter = 1, 8 bits
        self.add_val(SCI_MFG_CODE, 2)       # Field 7: Value of requested parameter = mfg code, 16 bits
        self.add_byte(3)                    # Field 8: Field number of requested parameter = 3, 8 bits
        self.add_val(SCI_INDUSTRY_CODE, 1)  # Field 9: Value of requested parameter = 4 (Marine), 8 bits

    def make_command(self, field, value):
        self.add_byte(1)                    # Field 1: Request Group Function Code = 1 (Command Message), 8 bits
        self.add_val(self.grp_pgn, 3)        # Field 2: PGN = 130900, 24 bits
        self.add_byte(0xff)                 # Field 3: Priority Setting = 0x8 (do not change priority), 4 bits
                                            # Field 4: Reserved =0xf, 4 bits
        self.add_byte(3)                    # Field 5: Number of Pairs of Request Parameters to follow = 3, 8 bits
        self.add_byte(1)                    # Field 6: Field number of requested parameter = 1, 8 bits
        self.add_val(SCI_MFG_CODE, 2)       # Field 7: Value of requested parameter = mfg code, 16 bits
        self.add_byte(3)                    # Field 8: Field number of requested parameter = 3, 8 bits
        self.add_val(SCI_INDUSTRY_CODE, 1)  # Field 9: Value of requested parameter = 4 (Marine), 8 bits
        self.add_byte(field)                 # Field 10: Field number of commanded parameter = 4, 8 bits
        self.add_val(value, 2)              # Field 11: Value of commanded parameter = AWSOffset, 16 bits

    def set_awa(self, awa_deg):
        self.make_command(4, int(awa_deg * math.pi / 180 * AWA_SCALE))

    def reset_awa(self):
        self.make_command(4, 0xfffe)

    def set_aws(self, aws_perc):
        aws_factor = 1 + aws_perc / 100
        self.make_command(5, int(aws_factor * AWS_SCALE))

    def reset_aws(self):
        self.make_command(5, 0xfffe)

    def set_sow(self, speed_perc):
        speed_factor = 1 + speed_perc / 100
        self.make_command(4, int(speed_factor * SOW_SCALE))

    def reset_sow(self):
        self.make_command(4, 0xfffe)


def calibrate(args):

    request_sent = False
    cal_sent = False

    if args.reset_sow or args.get_sow or args.sow is not None:
        cal_mhu = False
    else:
        cal_mhu = True

    try:
        with serial.Serial(args.port) as ser:
            while True:
                line = ser.readline()

                if not request_sent:
                    if cal_mhu:
                        msg = GroupFunction(MHU_CALIBRATION_PGN, PRIORITY, SRC, BCAST_DST)
                    else:
                        msg = GroupFunction(SPEED_CALIBRATION_PGN, PRIORITY, SRC, BCAST_DST)

                    msg.make_request()

                    send_msg(msg, ser)
                    request_sent = True

                parsed_msg = parse_line(line)
                if parsed_msg is not None:
                    rcvd_pgn = parsed_msg['pgn']
                    if rcvd_pgn == MHU_CALIBRATION_PGN or rcvd_pgn == SPEED_CALIBRATION_PGN:
                        if not cal_sent:
                            dst = parsed_msg['src']  # Can not broadcast command
                            msg = None
                            if args.awa is not None:
                                msg = GroupFunction(MHU_CALIBRATION_PGN, PRIORITY, SRC, dst)
                                msg.set_awa(args.awa)
                            elif args.aws is not None:
                                msg = GroupFunction(MHU_CALIBRATION_PGN, PRIORITY, SRC, dst)
                                msg.set_aws(args.aws)
                            elif args.sow is not None:
                                msg = GroupFunction(SPEED_CALIBRATION_PGN, PRIORITY, SRC, dst)
                                msg.set_sow(args.sow)
                            elif args.reset_aws:
                                msg = GroupFunction(MHU_CALIBRATION_PGN, PRIORITY, SRC, dst)
                                msg.reset_aws()
                            elif args.reset_awa:
                                msg = GroupFunction(MHU_CALIBRATION_PGN, PRIORITY, SRC, dst)
                                msg.reset_awa()
                            elif args.reset_sow:
                                msg = GroupFunction(SPEED_CALIBRATION_PGN, PRIORITY, SRC, dst)
                                msg.reset_sow()

                            if msg is not None:
                                send_msg(msg, ser)
                                cal_sent = True
                                request_sent = False  # Read calibration values again to check if it was accepted
                            else:
                                break
                        else:
                            break

    except IOError as error:
        print(f'{error}')


def send_msg(msg, ser):
    raw_msgs = msg.get_msgs()
    # for raw_msg in raw_msgs:
    #     print(f'{raw_msg}')
    for raw_msg in raw_msgs:
        print(f'Sending [{raw_msg}]')
        ser.write(raw_msg)


def parse_mhu_cal(data_ascii):
    print(data_ascii)
    data = [int(x, 16) for x in data_ascii]
    # print(data)
    dlc = data[0]
    pi = (data[1]) | (data[2] << 8)

    awa_cal = (data[3]) | (data[4] << 8)
    aws_cal = (data[5]) | (data[6] << 8)
    print(f'len={dlc} pi={pi} awa_cal=0x{awa_cal:04x} aws_cal=0x{aws_cal:04x}')
    awa_deg = awa_cal / AWA_SCALE * 180 / math.pi
    aws_perc = (aws_cal / AWS_SCALE - 1) * 100
    print(f'awa_deg = {awa_deg:.1f}° aws_perc={aws_perc:.1f}%')
    return awa_cal, aws_cal


def parse_speed_cal(data_ascii):
    print(data_ascii)
    data = [int(x, 16) for x in data_ascii]
    # print(data)
    dlc = data[0]
    pi = (data[1]) | (data[2] << 8)

    sow_cal = (data[3]) | (data[4] << 8)
    print(f'len={dlc} pi={pi} sow_cal=0x{sow_cal:04x}')
    sow_perc = (sow_cal / SOW_SCALE - 1) * 100
    print(f'sow_perc={sow_perc:.1f}%')
    return sow_perc


def parse_line(line):
    t = line.decode('ascii').split()
    # print(t)
    can_id = int(t[2], 16)
    prio, pgn, src, dst = can_id_to_n2k(can_id)
    # print(f'prio={prio}, pgn={pgn}, src={src}, dst={dst}')
    if pgn == MHU_CALIBRATION_PGN:
        awa_cal, aws_cal = parse_mhu_cal(t[4:])
        return {
            'prio': prio,
            'pgn': pgn,
            'src': src,
            'dst': dst,
            'data': {
                'aws_cal': aws_cal,
                'awa_cal': awa_cal
            }
        }
    elif pgn == SPEED_CALIBRATION_PGN:
        sow_cal = parse_speed_cal(t[4:])
        return {
            'prio': prio,
            'pgn': pgn,
            'src': src,
            'dst': dst,
            'data': {
                'sow_cal': sow_cal,
            }
        }
    return None


if __name__ == '__main__':
    cmdparser = argparse.ArgumentParser(description="Send calibartion to MHU to NMEA Unit using YDNU gateway")
    cmdparser.add_argument('--port',  help="Port name of YDNU", required=True)
    cmdparser.add_argument('--awa', type=float, help="AWA calibration (degrees)")
    cmdparser.add_argument('--aws', type=float, help="AWS calibration (percent)")
    cmdparser.add_argument('--sow', type=float, help="AWS calibration (percent)")
    cmdparser.add_argument('--get-sow', action='store_true', help="Read SOW calibration")
    cmdparser.add_argument('--reset-aws', action='store_true', help="Reset AWS calibration")
    cmdparser.add_argument('--reset-awa', action='store_true', help="Reset AWS calibration")
    cmdparser.add_argument('--reset-sow', action='store_true', help="Reset AWS calibration")

    calibrate(cmdparser.parse_args())

