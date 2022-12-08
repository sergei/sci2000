import numpy as np
import matplotlib.pyplot as plt


def simulate_mhu_outputs(deg):
    """ Simulates the MHU ouput per B&G Masthead unit testing information
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

    # Now find the corresponding angles
    est_red_deg = np.rad2deg(np.arccos(-est_red_u))
    est_green_deg = np.rad2deg(np.arcsin(est_green_u)) + 210
    est_blue_deg = np.mod(np.rad2deg(np.arcsin(est_blue_u)) + 330, 360)

    plt.figure()
    plt.plot(deg[i_red], est_red_deg[i_red], 'r.')
    plt.plot(deg[i_blue], est_blue_deg[i_blue], 'b.')
    plt.plot(deg[i_green], est_green_deg[i_green], 'g.')
    plt.xlabel('Degrees')
    plt.ylabel('Degrees')
    plt.title('Arcsin/Arccos  of inputs (in valid rages only)')


def decode_angle(red, green, blue):
    """ Decoding angle function in a form close to C++ implementation"""
    est_a = (red + blue + green) / 3
    est_red_u = red / est_a - 1
    est_green_u = green / est_a - 1
    est_blue_u = blue / est_a - 1

    if est_green_u < 0 and est_blue_u >= 0:
        angle = np.arccos(-est_red_u)
    elif est_blue_u < 0 and est_red_u >= 0:
        angle = np.arcsin(est_green_u) + 7 * np.pi / 6
    else:
        angle = np.arcsin(est_blue_u) + 11 * np.pi / 6

    angle = np.mod(angle, np.pi * 2)
    return angle


def simulate_fw():
    deg = np.linspace(0, 359, num=360)
    blue, green, red = simulate_mhu_outputs(deg)

    angles = np.empty(len(deg))
    for i in range(len(deg)):
        angles[i] = decode_angle(red[i], green[i], blue[i])

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
    plt.plot(deg, np.rad2deg(angles), 'k')
    plt.xlabel('Degrees')
    plt.ylabel('Degrees')
    plt.title('Decoded angle')


if __name__ == '__main__':
    plot_awa_curves()
    simulate_fw()
    plt.show()
