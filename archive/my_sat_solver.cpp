#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "sat_solver.hpp"

using namespace std;

pair<bool, size_t> str2literal(string string)
{
    bool type = (string[0] != '!');
    size_t id = stoull(string.substr((size_t)!type));
    return {type, id};
}

template <typename Container>
vector<vector<pair<bool, size_t>>> str2vec_formatter(Container source)
{
    vector<vector<pair<bool, size_t>>> res;
    for (const string &str : source)
    {
        vector<pair<bool, size_t>> cur_clause;
        size_t start = 0;
        size_t end = 0;
        while ((end = str.find(' ', start)) != string::npos)
        {
            cur_clause.push_back(str2literal(str.substr(start, end - start)));
            start = end + 1;
        }
        if (start < str.size())
            cur_clause.push_back(str2literal(str.substr(start)));
        res.push_back(cur_clause);
    }
    return res;
}

int main()
{
    vector<vector<pair<bool, size_t>>> test1{
        {{false, 0}},
        {{true, 0}, {false, 1}, {false, 2}},
        {{true, 1}, {false, 2}, {false, 3}},
        {{true, 3}},
    };
    vector<string> test2_str{
        "!2 !3 !4 5",
        "!1 2 3 4 5 !6",
        "!1 !5 6",
        "!5 7",
        "!1 !6 !7",
        "!1 !3 5",
        "!1 !4 5",
        "!1 !3 5",
        "!1 !5"};

    vector<string> test3_str{
        "1 4",
        "1 !3 !8",
        "1 8 12",
        "2 11",
        "!7 !3 9",
        "!7 8 !9",
        "7 8 !10",
        "7 10 !12"};

    auto test = str2vec_formatter(test3_str);
    SATSolver sat_solver;
    sat_solver.initiate(test.begin(), test.end());
    sat_solver.solve();
}