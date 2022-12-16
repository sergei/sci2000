def n2k_to_can_id(priority, pgn, src, dst):
    can = (pgn >> 8) & 0x00FF

    if can < 240:  # PDU1 format
        if (pgn & 0xff) != 0:
            return 0  # for PDU1 format PGN lowest byte has to be 0 for the destination.
        return (priority & 0x7) << 26 | pgn << 8 | dst << 8 | src
    else:  # PDU2 format
        return (priority & 0x7) << 26 | pgn << 8 | src


def can_id_to_n2k(can_id):
    can_id_pf = (can_id >> 16) & 0x00FF
    can_id_ps = (can_id >> 8) & 0x00FF
    can_id_dp = (can_id >> 24) & 1

    src = can_id >> 0 & 0x00FF
    prio = ((can_id >> 26) & 0x7)

    if can_id_pf < 240:
        # /* PDU1 format, the PS contains the destination address */
        dst = can_id_ps
        pgn = (can_id_dp << 16) | ((can_id_pf) << 8)
    else:
        # /* PDU2 format, the destination is implied global and the PGN is extended */
        dst = 0xff
        pgn = (can_id_dp << 16) | (can_id_pf << 8) |can_id_ps

    return prio, pgn, src, dst


if __name__ == '__main__':
    hdr = n2k_to_can_id(1, 130900, 15, 255)
    print(f'{hdr:x}')
