import argparse
import re

import numpy as np
import matplotlib.pyplot as plt

HIGH_WEIGHT = 1
LOW_WEIGHT = 0.2


def simulate_mhu_outputs(deg):
    """ Simulates the MHU output per B&G Masthead unit testing information
    see doc/B&G FAQ-MHU Wiring and Tests.pdf
    :arg deg array of degrees
    :returns red, green, blue - voltage on the corresponding wires
    """
    red_u = - np.cos(np.deg2rad(deg))
    green_u = - np.sin(np.deg2rad(deg - 30))
    blue_u = np.sin(np.deg2rad(deg + 30))
    a = 3.2
    red = a * (red_u + 1)
    green = a * (green_u + 1)
    blue = a * (blue_u + 1)
    return blue, green, red


def plot_awa_curves():
    deg = np.linspace(0, 359, num=360)

    blue, green, red = simulate_mhu_outputs(deg)

    plt.figure()
    plt.plot(deg, red, 'r.')
    plt.plot(deg, green, 'g.')
    plt.plot(deg, blue, 'b.')
    plt.plot(deg, red + blue + green, 'k.')
    plt.legend(['Pin5', 'Pin4', 'Pin6', 'Sum of voltages'])
    plt.xlabel('Degrees')
    plt.ylabel('Volts')
    plt.title('Simulated MHU output')

    est_a = (red + blue + green) / 3
    est_red_u = red / est_a - 1
    est_green_u = green / est_a - 1
    est_blue_u = blue / est_a - 1

    plt.figure()
    plt.plot(deg, est_red_u, 'r.')
    plt.plot(deg, est_green_u, 'g.')
    plt.plot(deg, est_blue_u, 'b.')
    plt.plot(deg, est_red_u + est_blue_u + est_green_u, 'k')
    plt.xlabel('Degrees')
    plt.title('Normalized MHU output')

    # Find three 120 degree zones per each input where this input is monotonous

    i_red = np.logical_and(est_green_u < 0, est_blue_u >= 0)
    i_green = np.logical_and(est_blue_u < 0, est_red_u >= 0)
    i_blue = np.logical_and(est_red_u < 0, est_green_u >= 0)

    plt.figure()
    plt.plot(deg[i_red], est_red_u[i_red], 'r.')
    plt.plot(deg[i_green], est_green_u[i_green], 'g.')
    plt.plot(deg[i_blue], est_blue_u[i_blue], 'b.')
    plt.xlabel('Degrees')
    plt.title('One input per its 120 degree interval')


def decode_angle(red, green, blue):
    """ Decoding angle function in a form close to C++ implementation"""
    est_a = (red + blue + green) / 3
    est_red_u = scale_adc(red, est_a)
    est_green_u = scale_adc(green, est_a)
    est_blue_u = scale_adc(blue, est_a)

    angle_red = angle_red_0_180(est_red_u)
    angle_green = angle_green_120_300(est_green_u)
    angle_blue = angle_blue_240_060(est_blue_u)

    # Divide into 120 degree zones
    if est_green_u < 0 and est_blue_u >= 0:  # [30; 150] degrees
        angle = angle_red
    elif est_blue_u < 0 and est_red_u >= 0:  # [150; 270] degrees
        angle = angle_green
    else:      # [270; 30] degrees
        angle = angle_blue

    # Divide into 60 degree zones and estimate angle within each zone using mostly the input in [-0.5;0.5] range

    if angle < 1 * np.pi / 3:  # [0; 60] degrees: green is most linear here
        angle_red = angle_red_0_180(est_red_u)
        angle_green = angle_green_300_120(est_green_u)
        angle_blue = angle_blue_240_060(est_blue_u)
    elif angle < 2 * np.pi / 3:  # [60; 120] degrees: red is most linear here
        angle_red = angle_red_0_180(est_red_u)
        angle_green = angle_green_300_120(est_green_u)
        angle_blue = angle_blue_60_240(est_blue_u)
    elif angle < 3 * np.pi / 3:  # [120; 180] degrees: blue is most linear here
        angle_red = angle_red_0_180(est_red_u)
        angle_green = angle_green_120_300(est_green_u)
        angle_blue = angle_blue_60_240(est_blue_u)
    elif angle < 4 * np.pi / 3:  # [180; 240] degrees: green is most linear here
        angle_red = angle_red_180_360(est_red_u)
        angle_green = angle_green_120_300(est_green_u)
        angle_blue = angle_blue_60_240(est_blue_u)
    elif angle < 5 * np.pi / 3:  # [240; 300] degrees : red is most linear here
        angle_red = angle_red_180_360(est_red_u)
        angle_green = angle_green_120_300(est_green_u)
        angle_blue = angle_blue_240_060(est_blue_u)
    else:  # [300; 360] degrees: blue is most linear here
        angle_red = angle_red_180_360(est_red_u)
        angle_green = angle_green_300_120(est_green_u)
        angle_blue = angle_blue_240_060(est_blue_u)

    red_w = get_weight(est_red_u)
    green_w = get_weight(est_green_u)
    blue_w = get_weight(est_blue_u)

    angle = get_angle_w(angle_blue, angle_green, angle_red, blue_w, green_w, red_w)
    return angle, angle_red, angle_green, angle_blue


def get_weight(v):
    # Weight should be highest close to 0 and lowest close to 1 and - 1
    weight = (1 - np.abs(v)) * 10
    if weight < 1:  # Avoid division by zero
        weight = 1
    return weight


def scale_adc(v, ampl):
    # Scale ADC value to [-1;1] range
    s = v / ampl - 1
    if s > 1:
        s = 1
    elif s < -1:
        s = -1
    return s


def angle_red_0_180(x):
    angle_red = np.mod(np.arcsin(x) + 3 * np.pi / 6, np.pi * 2)
    return angle_red


def angle_red_180_360(x):
    angle_red = np.mod(- np.arcsin(x) - 3 * np.pi / 6, np.pi * 2)
    return angle_red


def angle_green_120_300(x):
    angle_green = np.mod(np.arcsin(x) - 5 * np.pi / 6, np.pi * 2)
    return angle_green


def angle_green_300_120(x):
    angle_green = np.mod(- np.arcsin(x) + 1 * np.pi / 6, np.pi * 2)
    return angle_green


def angle_blue_60_240(x):
    angle_blue = np.mod(- np.arcsin(x) + 5 * np.pi / 6, np.pi * 2)
    return angle_blue


def angle_blue_240_060(x):
    angle_blue = np.mod(np.arcsin(x) - 1 * np.pi / 6, np.pi * 2)
    return angle_blue


def get_angle_w(angle_blue, angle_green, angle_red, blue_w, green_w, red_w):
    # Adjust angles so the delta between them is not too big
    if angle_green - angle_blue > np.pi:
        angle_green -= np.pi * 2
    elif angle_blue - angle_green > np.pi:
        angle_green += np.pi * 2

    if angle_red - angle_green > np.pi:
        angle_red -= np.pi * 2
    elif angle_green - angle_red > np.pi:
        angle_red += np.pi * 2

    angle = (angle_green * green_w + angle_blue * blue_w + angle_red * red_w) / (blue_w + green_w + red_w)
    angle = np.mod(angle, np.pi * 2)
    return angle


def simulate_fw():
    deg = np.linspace(0, 359, num=360)
    blue, green, red = simulate_mhu_outputs(deg)

    angles = np.empty(len(deg))
    angles_red = np.empty(len(deg))
    angles_green = np.empty(len(deg))
    angles_blue = np.empty(len(deg))
    for i in range(len(deg)):
        angles[i], angles_red[i], angles_green[i], angles_blue[i] = decode_angle(red[i], green[i], blue[i])

    plt.figure()
    plt.subplot(2, 1, 1)
    plt.plot(deg, red, 'r.')
    plt.plot(deg, green, 'g.')
    plt.plot(deg, blue, 'b.')
    plt.legend(['Pin5', 'Pin4', 'Pin6', 'Sum of voltages'])
    plt.xlabel('Degrees')
    plt.ylabel('Volts')
    plt.title('Simulated MHU output')
    plt.subplot(2, 1, 2)

    plt.plot(deg, np.rad2deg(angles_red), 'r.')
    plt.plot(deg, np.rad2deg(angles_green), 'g.')
    plt.plot(deg, np.rad2deg(angles_blue), 'b.')

    plt.plot(deg, np.rad2deg(angles), 'k')

    plt.xlabel('Degrees')
    plt.ylabel('Degrees')
    plt.title('Decoded angle')


def unescape(line):
    # Remover terminal escape sequences
    line = re.sub(r'\x1b\[[0-9;]*m', '', line)
    return line


def proces_log(args):
    if args.log_file is None:
        return

    adc_r = []
    adc_g = []
    adc_b = []

    raw_a = []
    est_a = []
    fw_r = []
    fw_g = []
    fw_b = []
    fw_raw_awa = []
    fw_est_awa = []

    got_adc = False
    with open(args.log_file, 'rt', encoding='latin-1') as log_file:
        for line in log_file:
            line = unescape(line).strip()
            if 'AWA_ADC,' in line:
                t = line.split(',')
                if len(t) < 7:
                    continue
                adc_r.append(float(t[2]))
                adc_g.append(float(t[4]))
                adc_b.append(float(t[6]))
                got_adc = True
            if 'AWA,dt_sec,' in line and got_adc:
                t = line.split(',')
                if len(t) < 17:
                    continue
                raw_a.append(float(t[4]))
                est_a.append(float(t[6]))
                fw_r.append(float(t[8]))
                fw_g.append(float(t[10]))
                fw_b.append(float(t[12]))
                fw_raw_awa.append(float(t[14]))
                fw_est_awa.append(float(t[16]))
                got_adc = False

    # Now plot python vs fw

    angles = np.empty(len(adc_r))
    angles_red = np.empty(len(adc_r))
    angles_green = np.empty(len(adc_r))
    angles_blue = np.empty(len(adc_r))
    for i in range(len(adc_r)):
        angles[i], angles_red[i], angles_green[i], angles_blue[i] = decode_angle(adc_r[i], adc_g[i], adc_b[i])

    plt.figure()
    plt.subplot(2, 1, 1)
    plt.plot(adc_r, 'r.')
    plt.plot(adc_g, 'g.')
    plt.plot(adc_b, 'b.')
    plt.plot(raw_a, 'k.')
    plt.legend(['Pin5', 'Pin4', 'Pin6', 'Sum of ADC values'])
    plt.title('ADC values')
    plt.subplot(2, 1, 2)

    plt.plot(np.rad2deg(angles_red), 'r.')
    plt.plot(np.rad2deg(angles_green), 'g.')
    plt.plot(np.rad2deg(angles_blue), 'b.')

    plt.plot(fw_raw_awa, 'y.')
    plt.plot(np.rad2deg(angles), 'ko')

    plt.ylabel('Degrees')
    plt.title('AWA estimate')
    plt.legend(['asin(R)', 'asin(G)', 'asin(B)',  'Raw FW', 'Python'])


if __name__ == '__main__':

    parser = argparse.ArgumentParser(fromfile_prefix_chars='@')
    parser.add_argument("--log-file", help="recorded log file", required=False)

    plot_awa_curves()
    simulate_fw()

    proces_log(parser.parse_args())

    plt.show()
