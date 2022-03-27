#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <queue>
#include <cassert>
#include <array>
#include "../utils/utility.hpp"

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
        SATSolver &sat_solver;

        // true if the literal is `variable`,
        // false if the literal is `Not(variable)`
        const bool literal_type;

    public:
        VariableValue get_value() const;
        VariableID get_variable_id() const;
        auto &get_variable() const
        {
            return sat_solver.variables[get_variable_id()];
        }

        Literal(SATSolver &sat_solver, VariableID variable, bool literal_type);
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
        template <typename Iterator>
        Clause(SATSolver &sat_solver, ClauseID clauseID);

        auto add_literal(Literal liter);

        /**
         * @brief Update the values of literals in the clause according to the assignment of variables;
         *
         * NOTE The update will NOT add literals to the clause
         *
         */
        void update();

        void assign(VariableID variableID, VariableValue variableValue)
        {
            auto oldValue = sat_solver.variables[variableID].value;
            // assert()
        }

        auto get_clause_id() const
        {
            return clauseID;
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

        auto add_clause(ClauseID);
    };

    class ImplicationGraph
    {
    private:
        struct Node
        {
            VariableID variableID;
            size_t dicision_level;
            bool is_dicision_node;

            unordered_map<Index, ClauseID> out_edges, in_edges;
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
        size_t get_decision_level()
        {
            return decision_points.size() - 1;
        }

        vector<Node> stack;

        void push_propagate(VariableID variableID)
        {
            var2pos[variableID] = stack.size();
            stack.push_back({variableID, get_decision_level(), false, {}, {}});
        }

        void push_decision_node(VariableID variableID)
        {
            var2pos[variableID] = stack.size();
            stack.push_back({variableID, get_decision_level(), true, {}, {}});
            decision_points.push_back({stack.size() - 1});
        }

        void add_edge(VariableID from, VariableID to, ClauseID clauseID)
        {
            Index from_idx = var2pos[from];
            Index to_idx = var2pos[to];
            stack[from_idx].out_edges[to_idx] = clauseID;
            stack[to_idx].in_edges[from_idx] = clauseID;
        }

        void pop()
        {
            auto &to_pop = stack.back();
            auto to_pop_pos = stack.size() - 1;
            var2pos.erase(to_pop.variableID);
            assert(to_pop.out_edges.empty());
            for (auto &in_edge : to_pop.in_edges)
            {
                stack[in_edge.first].out_edges.erase(to_pop_pos);
            }

            if (to_pop.is_dicision_node)
            {
                assert(decision_points.back().decisionPos == to_pop_pos);
                decision_points.pop_back();
            }
        }

        Clause confilict_analysis(ClauseID conflict_clause)
        {
        }
    };

    vector<Clause> clauses;
    vector<Variable> variables;

    /**
     * @brief TODO make it a class. Implement function `change value of variables`
     *
     */
    array<unordered_set<VariableID>, 3> variables_by_value;

    vector<size_t> VarID2originalName;
    queue<ClauseID> unipropagateQueue;
    ImplicationGraph implicationGraph;

    /**
     * @brief Input specification: Container<Container<pair<bool, size_t>>>
     *
     * TODO Preprocess: currrently, we assert a variable appears in a clause at most once.
     *
     * @tparam Iterator
     * @param clause_first
     * @param clause_last
     */
    template <typename Iterator>
    void initiate(Iterator clause_first, Iterator clause_last);

    void update_clauses();

    void assign(VariableID variableID, VariableValue variableValue)
    {
        auto oldValue = variables[variableID].value;
        if (oldValue == variableValue)
            return;

        variables_by_value[oldValue].erase(variableID);
        variables_by_value[variableValue].insert(variableID);

        for (auto &clause : variables[variableID].clauses)
        {
        }
    }

    /**
     * @brief TODO Preprocess, unipropagate before the first decision.
     *
     * @return true SAT
     * @return false UNSAT
     */
    bool solve();
};