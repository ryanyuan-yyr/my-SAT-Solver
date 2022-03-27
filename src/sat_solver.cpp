#include "sat_solver.hpp"

SATSolver::Literal::Literal(SATSolver *sat_solver, VariableID variable, bool literal_type) : variableID(variable), sat_solver(sat_solver), literal_type(literal_type) {}

VariableValue SATSolver::Literal::get_value() const
{
    return get_value_if(sat_solver->get_variable(variableID).value);
}

SATSolver::VariableID SATSolver::Literal::get_variable_id() const
{
    return variableID;
}

SATSolver::Clause::Clause(SATSolver &sat_solver, ClauseID clauseID) : sat_solver(sat_solver), clauseID(clauseID)
{
}

pair<unordered_set<SATSolver::VariableID>::iterator, bool> SATSolver::Variable::add_clause(ClauseID clauseID)
{
    return clauses.insert(clauseID);
}

bool SATSolver::Clause::add_literal(VariableID variableID, bool literal_type)
{
    return literals_by_value[sat_solver.get_variable(variableID).value].insert(variableID).second &&
           literals.insert({variableID, Literal(&sat_solver, variableID, literal_type)}).second;
}

/**
 * @brief Do not change unipropagation queue
 *
 */
void SATSolver::Clause::update()
{
    array<vector<VariableID>, 3> inconsistent_set;
    for (size_t i = 0; i < 3; i++)
    {
        for (auto &literalID : literals_by_value[i])
        {
            const auto &literal = literals[literalID];
            if (literal.get_value() != i)
                inconsistent_set[i].push_back(literalID);
        }
    }

    for (size_t i = 0; i < 3; i++)
    {
        for (auto literalID : inconsistent_set[i])
        {
            literals_by_value[i].erase(literalID);
            assert(literals_by_value[literals[literalID].get_value()].insert(literalID).second);
        }
    }
}

SATSolver::Variable::Variable(SATSolver &sat_solver, VariableID variableID) : sat_solver(sat_solver), variableID(variableID), value(VariableValue::UNDECIDED)
{
}

void SATSolver::update_clauses()
{
    for (auto &clause : clauses)
        clause.update();
}

/**
 * @brief NOTE If a conflict occurs, the unipropagation queue may not be consistent.
 * NOTE During the unipropagation, the queue may be inconsistent. e.g, 2 clauses to unipropagate have the same unassigned literal.
 *
 * @return optional<SATSolver::ClauseID>
 */
optional<SATSolver::ClauseID> SATSolver::unipropagate()
{
    while (!unipropagateQueue.empty())
    {
        ClauseID to_propagate_clause_id = unipropagateQueue.front();
        auto &to_propagate_clause = get_clause(to_propagate_clause_id);

        // The conflict detection should be done at the moment when the last variable assignment in this clause happens.
        assert(!to_propagate_clause.is_conflict());

        if (to_propagate_clause.value() == TRUE)
        {
            // Nothing to do
        }
        else
        {
            assert(to_propagate_clause.get_literals_by_value(UNDECIDED).size() == 1);
            auto to_propagate_variable = to_propagate_clause.get_literals_by_value(UNDECIDED).begin().operator*();
            auto &to_propagate_literal = to_propagate_clause.get_literal(to_propagate_variable);
            bool to_assign_value = to_propagate_literal.get_literal_type();

            auto assign_result = assign(to_propagate_variable, to_assign_value);
            // For consistency, even if a conflict is detected, the implication graph should be update.
            implicationGraph.push_propagate(to_propagate_variable, to_propagate_clause_id);

            if (assign_result.has_value())
            {
                unipropagateQueue.clear();
                return assign_result;
            }
        }
        unipropagateQueue.pop_front();
    }
    return nullopt;
}

bool SATSolver::solve()
{
    while (!variables_by_value[UNDECIDED].empty())
    {
        if (unipropagateQueue.empty())
        {
            auto decision = decisionPolicy();
            assert(!assign(decision.first, decision.second).has_value());
            implicationGraph.push_decision_node(decision.first);
        }

        auto unipropagate_result = unipropagate();
        if (unipropagate_result.has_value())
        {
            auto conflict_result = implicationGraph.confilict_analysis(unipropagate_result.value());
            assert(!conflict_result.empty());
            if (conflict_result == vector<Index>{0} && !implicationGraph[0].is_decision_node())
                return false;
            else
            {
                // Assert that current decision level cannot be 0;
                // Otherwise conflict_result == {0} and stack[0] is not a decision node.
                assert(implicationGraph.get_decision_level() != 0);

                log_stream << "[Conflict analysis] ";
                for (auto pos : conflict_result)
                    log_stream << implicationGraph[pos].variableID << ", ";
                log_stream << endl;
                auto learnt_clause_id = clauses.size();
                Clause learnt_clause(*this, learnt_clause_id);
                for (auto pos : conflict_result)
                {
                    auto var_id = implicationGraph[pos].variableID;
                    learnt_clause.add_literal(var_id, !get_variable(var_id).value);
                }
                learnt_clause.update();
                for (auto literal : learnt_clause.get_literals())
                {
                    get_variable(literal.first).add_clause(learnt_clause_id);
                }

                add_clause(std::move(learnt_clause));
                assert(unipropagateQueue.size() == 0);
                unipropagateQueue.push_back(learnt_clause_id);
                /**
                 * @brief Backjump
                 *
                 */
                size_t backjump_decision_level;
                if (conflict_result.size() == 1)
                    backjump_decision_level = 0;
                else
                    backjump_decision_level = implicationGraph[std::max_element(conflict_result.cbegin() + 1, conflict_result.cend()).operator*()].decision_level;

                while (implicationGraph.get_decision_level() > backjump_decision_level)
                {
                    reset(implicationGraph.back().variableID);
                    implicationGraph.pop();
                }

                log_stream << "[Backjump] "
                           << "L" << backjump_decision_level << " "
                           << "stack depth: " << implicationGraph.size() << endl;
            }
        }
    }

    return true;
}