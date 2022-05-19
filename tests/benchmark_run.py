import subprocess
import os

testcase_root_dir = 'tests/testcases/'


class Benchmark:

    def __init__(self, dir_name: str, file_name_pattern: str, range: range,
                 expected_result: bool) -> None:
        self.dir_name = dir_name
        self.file_name_pattern = file_name_pattern
        self.range = range
        self.expected_result = expected_result


CBS_k3_n100_m403_b10 = Benchmark('CBS_k3_n100_m403_b10',
                                 'CBS_k3_n100_m403_b10_{}.cnf', range(0, 1000),
                                 True)
uf20 = Benchmark('uf20-91', 'uf20-0{}.cnf', range(1, 1001), True)
uf50 = Benchmark('uf50-218', 'uf50-0{}.cnf', range(1, 1001), True)
uf75 = Benchmark('uf75-325', 'uf75-0{}.cnf', range(1, 101), True)
uuf50 = Benchmark('UUF50.218.1000', 'uuf50-0{}.cnf', range(1, 1001), False)
uuf75 = Benchmark('UUF75.325.100', 'uuf75-0{}.cnf', range(1, 101), False)
uuf100 = Benchmark('uuf100-430', 'uuf100-0{}.cnf', range(1, 1001), False)

benchmarks = [
    CBS_k3_n100_m403_b10,
    # uf20,
    uf50,
    uf75,
    # uuf50,
    # uuf75,
    uuf100,
]

for benchmark in benchmarks:
    print(benchmark.dir_name)
    for i in benchmark.range:
        out = subprocess.Popen([
            './build/sat_solver',
            (testcase_root_dir + benchmark.dir_name + '/' +
             benchmark.file_name_pattern).format(i)
        ],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.DEVNULL)

        stdout, stderr = out.communicate()
        if benchmark.expected_result:
            if stdout != b'SAT\n':
                print("Assertion fail", i)
                os.abort()
        else:
            if stdout != b'UNSAT\n':
                print('Assertion fail', i)
                os.abort()
        if i % 100 == 0:
            print(i)
