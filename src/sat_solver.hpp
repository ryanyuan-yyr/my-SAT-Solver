#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <optional>
#include <queue>
#include <deque>
#include <iterator>
#include <array>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include "utility.hpp"

#ifndef SAT_SOLVER
#define SAT_SOLVER

using namespace std;

/**
 * @brief
 *
 * TODO Modify accessibility
 *
 */

class SATSolver
{
public:
    using ClauseID = size_t;
    using VariableID = size_t;
    using DecidedValue = bool;
    using Index = size_t;

    struct Literal;
    class Clause;
    class Variable;
    class ImplicationGraph;

    struct Literal
    {
    private:
        const VariableID variableID;
        SATSolver *sat_solver;

        // true if the literal is `variable`,
        // false if the literal is `Not(variable)`
        const bool literal_type;

    public:
        VariableValue get_value() const;
        VariableValue get_value_if(VariableValue variableValue) const;
        VariableID get_variable_id() const;
        Variable &get_variable() const;

        Literal(SATSolver *sat_solver, VariableID variable, bool literal_type);
        Literal() : variableID(0), sat_solver(nullptr), literal_type(false)
        {
            throw logic_error("The default constructor for SATSolver::Literal should never be called");
        }

        bool get_literal_type() const;
    };

    class Clause
    {
    private:
        SATSolver &sat_solver;
        ClauseID clauseID;

        // NOTE A variable should not appear more than once in a single clause
        // NOTE array[FALSE/TRUE/UNASSIGNED] are the variables whose LITERALS in this clause are true/false/unassigned.
        /**
         * value of literals -> literals_by_value <-> literals <-> variables
         *
         */
        array<unordered_set<VariableID>, 3> literals_by_value;
        unordered_map<VariableID, Literal> literals;

    public:
        Clause(SATSolver &sat_solver, ClauseID clauseID);

        bool add_literal(VariableID variableID, bool literal_type);

        /**
         * @brief Update the values of literals in the clause according to the assignment of variables;
         *
         * NOTE The update will NOT add literals to the clause
         *
         */
        void update();

    private:
        void change_assignment(VariableID variableID, VariableValue from, VariableValue to);

    public:
        /**
         * @brief `assign` will not remove clause from unipropagation queue
         *
         * @param variableID
         * @param variableValue
         * @return true
         * @return false
         */
        bool assign(VariableID variableID, bool b_variableValue);

        void reset(VariableID variableID);

        bool is_conflict();

        VariableValue value()
        {
            if (!literals_by_value[TRUE].empty())
                return TRUE;
            else if (!literals_by_value[UNASSIGNED].empty())
                return UNASSIGNED;
            else
                return FALSE;
        }

        const auto &get_literals_by_value(VariableValue literalValue)
        {
            return literals_by_value[literalValue];
        }

        size_t to_decide_num()
        {
            if (literals_by_value[TRUE].size() != 0)
                return 0;
            else
                return literals_by_value[UNASSIGNED].size();
        }

        auto get_clause_id() const
        {
            return clauseID;
        }

        const auto &get_literal(VariableID variableID)
        {
            return literals[variableID];
        }

        const auto &get_literals() const
        {
            return literals;
        }
    };

    class Variable
    {
    public:
        SATSolver &sat_solver;
        VariableID variableID;

        // The clauses containing this variable.
        unordered_set<ClauseID> clauses;

        VariableValue value;
        Variable(SATSolver &sat_solver, VariableID variableID);

        pair<unordered_set<SATSolver::VariableID>::iterator, bool> add_clause(ClauseID);
    };

    class ImplicationGraph
    {
    private:
        SATSolver &sat_solver;

    public:
        struct Node
        {
            VariableID variableID;
            size_t decision_level;

            /**
             * @brief If the node is a decision, derive_from is nullopt.
             * If the node is by unipropagation, derivate_from is the clause from which it derives.
             *
             */
            optional<ClauseID> derive_from;

            bool is_decision_node() const
            {
                return !derive_from.has_value();
            }
        };

        struct DecisionNode
        {
            // Position of the decision node in the stack
            Index decisionPos;
        };

    private:
        vector<Node> stack;

        /**
         * @brief decision_points[dl] info of decision node at level `dl`
         *
         */
        vector<DecisionNode> decision_points;
        unordered_map<VariableID, Index> var2pos;

    public:
        ImplicationGraph(SATSolver &sat_solver) : sat_solver(sat_solver) {}

        const auto &operator[](Index index) const
        {
            return stack[index];
        }

        const auto &back()
        {
            return stack.back();
        }

        auto size()
        {
            return stack.size();
        }

        size_t get_decision_level()
        {
            return decision_points.size();
        }

        void push_propagate(VariableID variableID, ClauseID derive_from)
        {
            var2pos[variableID] = stack.size();
            stack.push_back({variableID, get_decision_level(), derive_from});

            sat_solver.log_stream << "[Implication Graph] "
                                  << "L" << get_decision_level() << " " << sat_solver.VarID2originalName[variableID] << " " << sat_solver.get_variable(variableID).value << " \n";
        }

        void push_decision_node(VariableID variableID)
        {
            var2pos[variableID] = stack.size();
            decision_points.push_back({stack.size()});
            stack.push_back({variableID, get_decision_level(), nullopt});
            sat_solver.log_stream << "[Implication Graph] "
                                  << "L" << get_decision_level() << " " << sat_solver.VarID2originalName[variableID] << " " << sat_solver.get_variable(variableID).value << " \n";
        }

        void pop()
        {
            auto &to_pop = stack.back();
            if (!to_pop.derive_from)
            {
                claim(decision_points.back().decisionPos == stack.size() - 1);
                decision_points.pop_back();
            }
            stack.pop_back();
        }

        /**
         * @brief If the result is {0}, and stack[0] is not even a decision node, it is indicated that the CNF is unsat.
         *
         * @param conflict_clause
         * @return vector<Index>
         */
        vector<Index> confilict_analysis(ClauseID conflict_clause);
    };

    /**
     * @brief TODO Dummie dicision policy for now.
     *
     */
    class DecisionPolicy
    {
    private:
        SATSolver &sat_solver;

    public:
        DecisionPolicy(SATSolver &sat_solver) : sat_solver(sat_solver) {}

        pair<VariableID, bool> operator()() const
        {
            sat_solver.statistic.decisionNum++;
            return {sat_solver.variables_by_value[UNASSIGNED].begin().operator*(), true};

            // TODO
        }
    };

public:
    struct Statistic
    {
        std::chrono::nanoseconds time_cost;
        size_t decisionNum = 0;
        size_t backjumpNum = 0;
    };

private:
    friend class DecisionPolicy;

    ostream &log_stream;

    vector<size_t> VarID2originalName;
    vector<Clause> clauses;

    array<unordered_set<VariableID>, 3> variables_by_value;
    vector<Variable> variables;

    deque<ClauseID> unipropagate_queue;
    ImplicationGraph implication_graph;
    DecisionPolicy decision_policy;
    Statistic statistic;

public:
    SATSolver(ostream &log_stream = cerr) : log_stream(log_stream), implication_graph(*this), decision_policy(*this) {}

    /**
     * @brief Input specification: Container<Container<pair<bool, size_t>>>
     *
     * TODO Preprocess: currrently, we assert a variable appears in a clause at most once.
     *
     * NOTE Since it's a template, I put the definition in the header.
     *
     * @tparam Iterator
     * @param clause_first
     * @param clause_last
     */
    template <typename Iterator>
    void initiate(Iterator clause_first, Iterator clause_last)
    {
        unordered_map<size_t, VariableID> OriginalName2varID;
        Iterator clause_iter{clause_first};
        while (clause_iter != clause_last)
        {
            auto liter_iter = clause_iter->cbegin();
            ClauseID cur_clause_id = clauses.size();
            Clause cur_clause(*this, cur_clause_id);

            bool valid_clause = true;

            while (liter_iter != clause_iter->cend())
            {
                auto res = OriginalName2varID.find(liter_iter->second);
                VariableID cur_var_id;
                if (res == OriginalName2varID.end())
                // New variable
                {
                    cur_var_id = VarID2originalName.size();
                    OriginalName2varID[liter_iter->second] = cur_var_id;
                    VarID2originalName.push_back(liter_iter->second);
                    variables.push_back(Variable(*this, cur_var_id));
                    variables_by_value[UNASSIGNED].insert(cur_var_id);
                }
                else
                {
                    cur_var_id = res->second;
                }
                get_variable(cur_var_id).add_clause(cur_clause_id);
                if (!cur_clause.add_literal(cur_var_id, liter_iter->first))
                {
                    valid_clause = false;
                    break;
                }
                liter_iter++;
            }
            if (valid_clause)
                add_clause(std::move(cur_clause));
            else
            {
                for (auto &variable : variables)
                    variable.clauses.erase(cur_clause_id);
            }
            clause_iter++;
        }
    }
private:
    void add_clause(Clause &&clause)
    {
        if (clause.to_decide_num() == 1)
            unipropagate_queue.push_back(clause.get_clause_id());
        clauses.push_back(clause);
    }

    void update_clauses();

    /**
     * @brief `assign` should be the way and the only way to assign a non-unassigned value to a variable.
     *
     * @param variableID
     * @param variableValue
     * @return optional<ClauseID>
     */
    optional<ClauseID> assign(VariableID variableID, bool b_variableValue);

    void reset(VariableID variableID);

    Variable &get_variable(VariableID variableID)
    {
        return variables[variableID];
    }

    Clause &get_clause(ClauseID clauseID)
    {
        return clauses[clauseID];
    }

    /**
     * @brief returns the ID of the clause that causes the conflict if a conflict happens.
     *
     * @return optional<ClauseID>
     */
    optional<ClauseID> unipropagate();

public:
    /**
     * @brief
     *
     * @return true SAT
     * @return false UNSAT
     */
    bool solve();

    unordered_map<size_t, bool> get_result()
    {
        unordered_map<size_t, bool> result;
        for (auto &v : variables)
        {
            claim(v.value != UNASSIGNED);
            result[VarID2originalName[v.variableID]] = (v.value == TRUE);
        }
        return result;
    }

    auto get_statistics()
    {
        return statistic;
    }
};

#endif