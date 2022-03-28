from z3 import *

import time

for i in range(1, 1001):
    with open(f'tests/uf20-91/uf20-0{i}.mycnf', 'r') as input:
        cnf = []
        for line in input:
            cnf.append(
                Or(*[
                    Bool(var) if var[0] != '-' else Not(Bool(var[1:]))
                    for var in line.split()[:-1]
                ]))
    # print(cnf)
    s = Solver()
    s.add(cnf)
    T_start = time.process_time()
    assert (s.check() == sat)
    T_end = time.process_time()
    print((T_end - T_start) * 1E6)
