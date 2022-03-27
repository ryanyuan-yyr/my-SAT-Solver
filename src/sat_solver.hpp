#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <queue>
#include <deque>
#include <cassert>
#include <iterator>
#include <array>
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

    enum VariableValue
    {
        FALSE,
        TRUE,
        UNDECIDED,
    };

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
        VariableValue get_value_if(VariableValue variableValue) const
        {
            if (variableValue == VariableValue::UNDECIDED)
                return UNDECIDED;
            else
            {
                bool b_value = variableValue == VariableValue::TRUE;
                return exclusive_or(literal_type, b_value) ? FALSE : TRUE;
            }
        }
        VariableID get_variable_id() const;
        Variable &get_variable() const
        {
            return sat_solver->get_variable(get_variable_id());
        }

        Literal(SATSolver *sat_solver, VariableID variable, bool literal_type);
        Literal() : variableID(0), sat_solver(nullptr), literal_type(false)
        {
            throw logic_error("The default constructor for SATSolver::Literal should never be called");
        }

        bool get_literal_type() const
        {
            return literal_type;
        }
    };

    class Clause
    {
    private:
        SATSolver &sat_solver;
        ClauseID clauseID;

        // NOTE A variable should not appear more than once in a single clause
        // NOTE array[TRUE] are the variables whose LITERALS in this clause are true.
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

        bool assign(VariableID variableID, VariableValue variableValue)
        {
            auto oldValue = sat_solver.get_variable(variableID).value;
            if (oldValue != variableValue)
            {
                auto &literal = literals[variableID];
                literals_by_value[literal.get_value_if(oldValue)].erase(variableID);
                literals_by_value[literal.get_value_if(variableValue)].insert(variableID);

                if (to_decide_num() == 1)
                    sat_solver.unipropagateQueue.push_back(clauseID);
            }
            if (is_conflict())
                return false;
            else
                return true;
        }

        bool is_conflict()
        {
            return literals_by_value[UNDECIDED].size() == 0 && literals_by_value[TRUE].size() == 0;
        }

        VariableValue value()
        {
            if (!literals_by_value[TRUE].empty())
                return TRUE;
            else if (!literals_by_value[UNDECIDED].empty())
                return UNDECIDED;
            else
                return FALSE;
        }

        const auto &get_literals_by_value(VariableValue variableValue)
        {
            return literals_by_value[variableValue];
        }

        size_t to_decide_num()
        {
            if (literals_by_value[TRUE].size() != 0)
                return 0;
            else
                return literals_by_value[UNDECIDED].size();
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
        struct Node
        {
            VariableID variableID;
            size_t dicision_level;

            /**
             * @brief If the node is a decision, derive_from is nullopt.
             * If the node is by unipropagation, derivate_from is the clause from which it derives.
             *
             */
            optional<ClauseID> derive_from;
        };

        struct DecisionNode
        {
            // Position of the decision node in the stack
            Index decisionPos;
        };

        /**
         * @brief decision_points[dl] info of decision node at level `dl`
         *
         */
        vector<DecisionNode> decision_points;
        unordered_map<VariableID, Index> var2pos;

    public:
        ImplicationGraph(SATSolver &sat_solver) : sat_solver(sat_solver) {}

        vector<Node> stack;

        size_t get_decision_level()
        {
            return decision_points.size() - 1;
        }

        void push_propagate(VariableID variableID, ClauseID derive_from)
        {
            var2pos[variableID] = stack.size();
            stack.push_back({variableID, get_decision_level(), derive_from});
        }

        void push_decision_node(VariableID variableID)
        {
            var2pos[variableID] = stack.size();
            stack.push_back({variableID, get_decision_level(), nullopt});
            decision_points.push_back({stack.size() - 1});
        }

        void pop()
        {
            auto &to_pop = stack.back();
            if (!to_pop.derive_from)
            {
                assert(decision_points.back().decisionPos == stack.size() - 1);
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
        vector<Index> confilict_analysis(ClauseID conflict_clause)
        {
            auto &init_learnt_clause = sat_solver.get_clause(conflict_clause).get_literals();
            assert(init_learnt_clause.find(stack.back().variableID) != init_learnt_clause.end());
            unordered_set<Index> other_DL_nodes;
            priority_queue<Index, vector<Index>, std::greater<Index>> cur_DL_nodes;
            for (auto &varID_literal : init_learnt_clause)
            {
                auto node_pos = var2pos[varID_literal.first];
                if (stack[node_pos].dicision_level == get_decision_level())

                    cur_DL_nodes.push(node_pos);
                else
                    other_DL_nodes.insert(node_pos);
            }
            assert(!cur_DL_nodes.empty());

            while (cur_DL_nodes.size() > 1)
            {
                Index cur_node_pos = cur_DL_nodes.top();
                assert(stack[cur_node_pos].derive_from.has_value());
                cur_DL_nodes.pop();
                auto &cur_clause_literals = sat_solver.get_clause(stack[cur_node_pos].derive_from.value()).get_literals();
                for (auto &varID_literal : cur_clause_literals)
                {
                    if (varID_literal.first != stack[cur_node_pos].variableID)
                    {
                        if (stack[cur_node_pos].dicision_level == get_decision_level())

                            cur_DL_nodes.push(cur_node_pos);
                        else
                            other_DL_nodes.insert(cur_node_pos);
                    }
                }
            }
            assert(cur_DL_nodes.size() == 1);
            vector<Index> learnt_clause_pos;
            learnt_clause_pos.push_back(cur_DL_nodes.top());
            copy(other_DL_nodes.begin(), other_DL_nodes.end(), back_inserter(learnt_clause_pos));
        }
    };

    vector<size_t> VarID2originalName;
    vector<Clause> clauses;

    /**
     * @brief TODO make it a class. Implement function `change value of variables`
     *
     */
    array<unordered_set<VariableID>, 3> variables_by_value;
    vector<Variable> variables;

    deque<ClauseID> unipropagateQueue;
    ImplicationGraph implicationGraph;

    SATSolver() : implicationGraph(*this) {}

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
                }
                else
                {
                    cur_var_id = res->second;
                }
                get_variable(cur_var_id).add_clause(cur_clause_id);
                cur_clause.add_literal(cur_var_id, liter_iter->first); // TODO insert may fail.
                liter_iter++;
            }
            if (cur_clause.to_decide_num() == 1)
                unipropagateQueue.push_back(cur_clause.get_clause_id());
            clauses.push_back(cur_clause);
            clause_iter++;
        }
    }

    void update_clauses();

    optional<ClauseID> assign(VariableID variableID, VariableValue variableValue)
    {
        auto oldValue = get_variable(variableID).value;
        if (oldValue == variableValue)
            return nullopt;

        variables_by_value[oldValue].erase(variableID);
        variables_by_value[variableValue].insert(variableID);

        optional<ClauseID> conflict_clause;
        // For consistency, even if a conflict is detected, the assignment should be performed for all clauses.
        for (auto &clauseID : get_variable(variableID).clauses)
            if (!get_clause(clauseID).assign(variableID, variableValue) && !conflict_clause.has_value())
                conflict_clause = clauseID;

        // Assignment to variable should be after Clause::asign, since Clause::asign uses the value of variables to determine the old value.
        get_variable(variableID).value = variableValue;

        return conflict_clause;
    }

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

    /**
     * @brief TODO Preprocess, unipropagate before the first decision.
     *
     * @return true SAT
     * @return false UNSAT
     */
    bool solve();
};

#endif