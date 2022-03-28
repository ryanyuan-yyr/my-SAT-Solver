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

struct TestcaseSpec
{
    string test_case_name;
    string test_name_pattern;
    size_t index_start;
    size_t index_end;
};

int main()
{
    string testcases_root{"tests/testcases/"};
    vector<TestcaseSpec> testcases{
        {"uf20-91", "uf20-0", 1, 1000},
        {"CBS_k3_n100_m403_b10", "CBS_k3_n100_m403_b10_", 0, 999}};
    size_t testcase_num = 1;
    for (size_t i = testcases[testcase_num].index_start; i <= testcases[testcase_num].index_end; i++)
    {
        ifstream input(testcases_root + testcases[testcase_num].test_case_name + '/' + testcases[testcase_num].test_name_pattern + to_string(i) + ".cnf");
        claim(("Input file open", input));
        auto test = DIMACS2vec(input);
        SATSolver sat_solver;
        sat_solver.initiate(test.first.begin(), test.first.end());
        auto start = high_resolution_clock::now();
        claim(("satisfy", sat_solver.solve()));
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<std::chrono::microseconds>(stop - start);
        cout << duration.count() << endl;

        auto result = sat_solver.get_result();
        for (auto &clause : test.first)
        {
            bool cur_clause_assign = false;
            for (auto &literal : clause)
            {
                cur_clause_assign |= !(literal.first ^ result[literal.second]);
            }
            claim(cur_clause_assign);
        }

        ofstream formatted_cnf(testcases_root + testcases[testcase_num].test_case_name + '/' + testcases[testcase_num].test_name_pattern + to_string(i) + "_formatted.cnf");
        for (auto &clause : test.first)
        {
            for (auto &literal : clause)
            {
                if (!literal.first)
                    formatted_cnf << "-";
                formatted_cnf << literal.second << " ";
            }
            formatted_cnf << "0\n";
        }
    }
}