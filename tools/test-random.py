#!/usr/bin/python

import time
import sys

seeds = [
    [8, 137],
    [2, 3, 29, 457],
    [2, 9, 125, 1019],
    [8, 49, 577, 4157],
    [4, 3, 7, 157, 461],
    [2, 37, 827],
    [2, 3, 173],
    [2, 3, 7, 17, 73],
    [2, 49],
    [4],
    [2, 3, 197],
    [2, 729, 11, 37],
    [2, 3, 19],
    [4, 3, 131],
    [2, 3, 17],
    [2, 3, 19, 53, 3121],
    [2, 17, 383],
    [2, 31, 3209],
]

def seeded_rnd():
    t = int(time.time()*(2**20 - 1))
    def _rnd(t):
        sa = seeds[t % len(seeds)]
        return sa[(t + sa[0] + len(sa)//2) % len(sa)]
    r = _rnd(t)
    n = (13*(r + _rnd(r + 5)) + (t % 3)) % 31
    if n < 5: n += 5
    for i in range(1, n):
        r = (r * 31 + _rnd(r + t + n)) & 0xFFFFFFFFFFFFFFF
    return r

for i in range(0, int(sys.argv[1])):
    print(seeded_rnd())
