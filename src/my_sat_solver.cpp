#include <iostream>
#include <vector>
#include "sat_solver.hpp"

using namespace std;

int main()
{
    vector<vector<pair<bool, size_t>>> test_cnf{
        {{false, 0}},
        {{true, 1}},
        {{true, 0}, {false, 1}}};
    SATSolver sat_solver;
    sat_solver.initiate(test_cnf.begin(), test_cnf.end());
    sat_solver.solve();
}