def n2k_to_can_id(priority, pgn, src, dst):
    can = (pgn >> 8) & 0x00FF

    if can < 240:  # PDU1 format
        if (pgn & 0xff) != 0:
            return 0  # for PDU1 format PGN lowest byte has to be 0 for the destination.
        return (priority & 0x7) << 26 | pgn << 8 | dst << 8 | src
    else:  # PDU2 format
        return (priority & 0x7) << 26 | pgn << 8 | src


if __name__ == '__main__':
    hdr = n2k_to_can_id(1, 130900, 15, 255)
    print(f'{hdr:x}')
