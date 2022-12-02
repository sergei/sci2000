#! /usr/bin/python
import os

import matplotlib.pyplot as pl
import matplotlib.ticker as plticker
import numpy as np

import argparse


# Press the green button in the gutter to run the script.
def process_capture(t, v):

    thr = 0.75
    low_ind = v < thr
    v_thr = np.ones(len(v)) * 5.5
    v_thr[low_ind] = 0

    fft_result = np.fft.fft(v)
    fft_result = abs(fft_result * fft_result.conjugate())
    fft_db = 10*np.log10(fft_result)
    freq = np.fft.fftfreq(len(v), d=t[1]-t[0])

    fig, ax = pl.subplots()
    # pl.plot(freq, fft_db, '.')
    baud = 9600 * 1
    bit = 1 / baud
    loc_minor = plticker.MultipleLocator(base=bit * 1e6)
    loc_major = plticker.MultipleLocator(base=bit * 1e6 * 10)
    ax.xaxis.set_minor_locator(loc_minor)
    ax.xaxis.set_major_locator(loc_major)
    pl.plot(t * 1e6, v, '.-')
    pl.plot(t * 1e6, v_thr, '-g')
    ax.grid(which='both')


def analyze_scope_capture(csv_dir):
    print(f'Analyzing {csv_dir}')

    for root, dirs, files in os.walk(csv_dir):
        for name in files:
            if name.lower().endswith('.csv'):
                full_name = os.path.join(root, name)
                print(f'Reading {full_name} ...')
                capture = np.loadtxt(full_name, delimiter=',', skiprows=2,  usecols=(0, 1))
                t = capture[:, 0]
                t_uniform = np.linspace(t[0], t[-1],  num=len(t))
                v = capture[:, 1]
                process_capture(t_uniform, v)
                pl.show()
                return


def parse_c4(aws, awa, v):
    awa_not_scaled = v[5]
    awa_deg_scale = 2 ** 14 / 180
    awa_recovered = awa_not_scaled / awa_deg_scale
    print(f'{v} aws={aws} awa={awa} awa_recovered={awa_recovered:0.1f}')


def parse_3c(aws, awa, v):
    print(f'{v} aws={aws} awa={awa} ')
    pass


def analyze_uart_capture(uart_file):
    print(f'Reading {uart_file} ...')
    with open(uart_file) as f:
        for line in f:
            t = line.split('|')
            if len(t) == 2:
                hdr = t[0].split()
                data = ''.join(t[1].split())
                aws = int(hdr[1])
                awa = int(hdr[3])
                # print(f'aws={aws} awa={awa} {data[8:10]} {data} ')

                payload = data[10:-2]
                w = []
                v = []
                for i in range(0, len(payload), 4):
                    d = payload[i:i + 4]
                    w.append(d)
                    v.append(int(d, 16) & 0xFFFF)
                print(f'{w}')

                if data[8:10] == 'c4':
                    parse_c4(aws, awa, v)
                elif data[8:10] == '3c':
                    parse_3c(aws, awa, v)


def analyze_captures(args):
    if args.cvs_dir is not None:
        analyze_scope_capture(args.cvs_dir)

    if args.uart_file is not None:
        analyze_uart_capture(args.uart_file)


if __name__ == '__main__':
    cmdparser = argparse.ArgumentParser(description="Analyze CSV captures of B&G NET protocol")
    cmdparser.add_argument('--cvs_dir', help="File with IQ samples")
    cmdparser.add_argument('--uart-file', help="File UART capture")

    analyze_captures(cmdparser.parse_args())
