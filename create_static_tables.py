"""
Recreate data plane behaviour by implement approximate squaring
"""
import argparse
import logging
import pathlib
import typing
import multiprocessing

import itertools
import numpy as np


def basetwo(x):
    return int(x, base=2)


logger = logging.getLogger(__name__)

square_table_type = typing.List[typing.Tuple[str, int, int, int, int]]
overflow_table_type = typing.List[typing.Tuple[int, int, int, int]]


def _compute_square_table_iteratio(args):
    N, n, m = args
    table = []
    for permutation in itertools.product('01', repeat=min(m - 1, N - n - 1)):
        bitmatch = '0' * n + '1' + ''.join(permutation)

        mask = max(0, N - n - m)
        match = basetwo(bitmatch) << mask

        min_log = basetwo(bitmatch + '0' * mask)
        max_log = basetwo(bitmatch + '1' * mask)
        values = np.log2(np.arange(min_log, max_log + 1, dtype=np.uint64))
        squares = values ** 2
        cubes = values ** 3

        cube = round(float(np.mean(cubes)))
        square = round(float(np.mean(squares)))
        value = round(float(np.mean(values)))

        # mean_error_tmp = np.abs(1 - (value / squares))
        # mean_error_count += len(mean_error_tmp)
        # mean_error_sum += np.sum(mean_error_tmp)
        #
        # max_error = max(
        #     max_error,
        #     mean_error_tmp[0],
        #     mean_error_tmp[-1],
        # )

        table.append((bitmatch, match, N - mask, value, square, cube))
    return table

def compute_square_table(bits_in: int, accuracy: int) -> square_table_type:
    """
    Implementation after Sharma et al. doi: 10.5555/3154630.3154637
    bits_in: the number of input bits
    accuracy: A measure for the accuracy. 6 ~= 1% error, 7 ~= 0.5% error

    Returns all permutations of the following form, where + is the concatenation operator
    0 is a zero bit, 1 is a one bit and x is the TCAM wildcard:

    0^n + 1 + (0|1)^min(m-1, N-n-1) + x^max(0, N-n-m) for 0 <= n < N

    For every rule, the mean of the square of all matching values is saved as its result.
    """
    logger.debug(f"Computing square tables for {bits_in} bits, accuracy {accuracy}")

    # N and m are the same variables as were used in the paper
    N = bits_in
    m = accuracy

    # table holds the final result in the form of (match, mask, result)
    table = []

    # helpers to log the maximum and mean error
    # max_error = 0
    # mean_error_sum = 0
    # mean_error_count = 0

    with multiprocessing.Pool() as pool:
        for res in pool.imap(_compute_square_table_iteratio, ((N, n, m) for n in range(N))):
            table.extend(res)

    # logger.debug("Maximum error: %f", max_error)
    # logger.debug("Mean error: %f", mean_error_sum / mean_error_count)

    return table


def build_header_length_table(ipv4: bool, ipv6: bool, tcp: bool, udp: bool):
    """
    Build a header to header length match table, as representing this
    computation on the data plane is quite difficult.
    This maps a L3- and L4-type as well as the IPv4 ihl field and TCP dataOffset
    field to the correct header length. IPv4 ihl and TCP dataOffset use ternary
    matching, therefore add a mask to these as well.
    Returns only specified options to support #ifdef ENABLE/DISABLE for all L3
    and L4 protocols on the P4 level, therefore can be directly inserted into
    the corresponding matchspec_t

    :returns List[(
        header_length,
        ipv4.valid, ihl, ihl_mask,               # only if ipv4 == True
        ipv6.valid,                              # only if ipv6 == True
        tcp.valid, dataOffset, dataOffset_mask,  # only if tcp == True
        udp.valid                                # only if udp == True
        )]
    """
    l3_opts = []
    if ipv4:
        for i in range(0x5, 0x10):
            l3_opts.append((i * 4, 1, i, 0xFF))
    if ipv6:
        new_opts = []
        for res, *o in l3_opts:
            new_opts.append((res, *o, 0))

        ipv4_opts = (0, 0, 0) if ipv4 else ()
        new_opts.append((0, *ipv4_opts, 1))
        l3_opts = new_opts

    l4_opts = []
    if tcp:
        for res, *o in l3_opts:
            for i in range(0x5, 0x10):
                l4_opts.append((res + i * 4, *o, 1, i, 0xFF))
    if udp:
        new_opts = []
        for res, *o in l4_opts:
            new_opts.append((res, *o, 0))
        l4_opts = new_opts

        tcp_opts = (0, 0, 0) if tcp else ()
        for res, *o in l3_opts:
            l4_opts.append((res + 8, *o, *tcp_opts, 1))

    return l4_opts


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('p4', type=pathlib.Path)
    parser.add_argument('c', type=pathlib.Path)
    args = parser.parse_args()

    size_table = compute_square_table(16, 8)
    iat_table = compute_square_table(32, 8)
    header_lengths = build_header_length_table(True, True, True, True)

    with args.p4.open('w') as f:
        f.write(f"""
#define BYTE_LOG_TABLE_SIZE {len(size_table)}
#define IAT_LOG_TABLE_SIZE {len(iat_table)}
#define HEADER_LENGTH_TABLE_SIZE {len(header_lengths)}
""")

    size_entries = ',\n'.join(f"{{{','.join(map(str, i[1:]))}}}" for i in size_table)
    iat_entries = ',\n'.join(f"{{{','.join(map(str, i[1:]))}}}" for i in iat_table)

    header_length_matches = ',\n'.join(f"{{{','.join(map(str, i[1:]))}}}" for i in header_lengths)
    header_length_actions = ',\n'.join(f"{{{i[0]}}}" for i in header_lengths)

    with args.c.open('w') as f:
        f.write(f"""
log_square_entry_t iat_log_square[] = {{
{iat_entries}
}};

log_square_entry_t size_log_square[] = {{
{size_entries}
}};

p4_pd_qoe_switch_header_length_table_match_spec_t header_length_match[] = {{
{header_length_matches}
}};

p4_pd_qoe_switch_header_length_hit_action_spec_t header_length_action[] = {{
{header_length_actions}
}};
""")


if __name__ == '__main__':
    main()
