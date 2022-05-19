#undef NDEBUG
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <cctype>
#include "sat_solver.hpp"

using namespace std::chrono;
using namespace std;

pair<vector<vector<pair<bool, size_t>>>, int> DIMACS2vec(istream &input)
{
    vector<vector<pair<bool, size_t>>> res;
    vector<pair<bool, size_t>> cur_clause;
    int var_num = 0;
    string tmp;
    while (input >> tmp)
    {
        if (tmp.empty() || isalpha(tmp[0]))
            input.ignore((unsigned)(-1), '\n');
        else if (tmp[0] == '%')
            return {res, var_num};
        else
        {
            int value = stoi(tmp);
            if (value == 0)
            {
                res.push_back(cur_clause);
                cur_clause.clear();
            }
            else
            {
                var_num = std::max(var_num, abs(value));
                if (value < 0)
                    cur_clause.push_back({false, -value});
                else
                    cur_clause.push_back({true, value});
            }
        }
    }
    return {res, var_num};
}

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        cout << "Usage: sat_solver [file]\nfile should be in .cnf format\n";
        return 0;
    }
    string input_file_name = argv[1];
    ifstream input(input_file_name);
    if (!input)
    {
        cout << "Failed to open input file" << endl;
        return -1;
    }
    auto test = DIMACS2vec(input);
    SATSolver sat_solver;
    sat_solver.initiate(test.first.begin(), test.first.end());
    bool solver_result = sat_solver.solve();

    // Check the assignment really satisfies the formula
    if (solver_result)
    {
        auto result_assignment = sat_solver.get_result();
        bool formula_value = true;
        for (auto &clause : test.first)
        {
            bool cur_clause_assign = false;
            for (auto &literal : clause)
            {
                cur_clause_assign |= !(literal.first ^ result_assignment[literal.second]);
            }
            formula_value &= cur_clause_assign;
        }
        if (formula_value != solver_result)
        {
            cout << "Assertion on result fails" << endl;
            return -1;
        }
        for (auto &&assign : result_assignment)
        {
            cerr << assign.first << " = " << assign.second << "\n";
        }
    }
    cout << (solver_result ? "SAT" : "UNSAT") << endl;
}