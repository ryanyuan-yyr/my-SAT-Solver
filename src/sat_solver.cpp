#include "sat_solver.hpp"

SATSolver::Literal::Literal(SATSolver *sat_solver, VariableID variable, bool literal_type) : variableID(variable), sat_solver(sat_solver), literal_type(literal_type) {}

SATSolver::VariableValue SATSolver::Literal::get_value() const
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

    if (to_decide_num() == 1)
        sat_solver.unipropagateQueue.push_back(clauseID);
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

            auto assign_result = assign(to_propagate_variable, static_cast<VariableValue>(optional2variableValue(to_assign_value)));
            // For consistency, even if a conflict is detected, the implication graph should be update.
            implicationGraph.push_propagate(to_propagate_variable, to_propagate_clause_id);

            if (assign_result.has_value())
            {
                /**
                 * TODO Maintain unipropagation consistency
                 *
                 */
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
    if (unipropagate().has_value())
        return false;
    return true;
}