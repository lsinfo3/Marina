import itertools
import json
import math

import numpy as np


def compute_log_table(N=32, m=6):
    counter = 0
    table = []
    for n in range(N):
        for j in itertools.product('01', repeat=min(m - 1, N - n - 1)):
            bitmatch = '0' * n + '1' + ''.join(j)
            varmatch = 'x' * max(0, N - n - m)
            min_log = int(bitmatch + '0' * max(0, N - n - m), 2)
            max_log = int(bitmatch + '1' * max(0, N - n - m), 2)
            value = np.mean(np.log(np.arange(min_log, max_log + 1)))
            print(bitmatch + varmatch, value)
            table.append((bitmatch + varmatch, value))
            counter += 1

    max_value = max(list(zip(*table))[1])
    lg = math.ceil(math.log2(max_value))
    scale = l - 1 - lg
    table = [(match, int(round(value * 2 ** scale))) for match, value in table]

    return table, counter, scale


def compute_exp_table(l=10, scale=4):
    table = []
    scale = 2 ** scale
    for i in range(2 ** l):
        exp = math.exp(i / scale)
        table.append((i, int(round(exp))))
    return table


tables = {
    'keys': [],
    'tables': {},
}
for i, (m, l) in enumerate((
        #        (3, 7),
        #        (4, 8),
        (6, 10),
)):
    print(m)
    log_table, counter, scale = compute_log_table(m=m)
    exp_table = compute_exp_table(l=l, scale=scale)

    tables['keys'].append((counter, 2 ** l))
    tables['tables'][len(tables['keys']) - 1] = (log_table, exp_table)

with open('log_tables.json', 'w') as f:
    json.dump(tables, f)
