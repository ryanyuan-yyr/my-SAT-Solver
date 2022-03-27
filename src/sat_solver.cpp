#include "sat_solver.hpp"

SATSolver::Literal::Literal(SATSolver *sat_solver, VariableID variable, bool literal_type) : sat_solver(sat_solver), variableID(variable), literal_type(literal_type) {}

SATSolver::VariableValue SATSolver::Literal::get_value() const
{
    auto &value = sat_solver->get_variable(variableID).value;
    if (value == VariableValue::UNDECIDED)
        return UNDECIDED;
    else
    {
        bool b_value = value == VariableValue::TRUE;
        return exclusive_or(literal_type, b_value) ? FALSE : TRUE;
    }
}

SATSolver::VariableID SATSolver::Literal::get_variable_id() const
{
    return variableID;
}

template <typename Iterator>
SATSolver::Clause::Clause(SATSolver &sat_solver, ClauseID clauseID) : sat_solver(sat_solver), clauseID(clauseID)
{
}

auto SATSolver::Clause::add_literal(VariableID variableID, bool literal_type)
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
}

SATSolver::Variable::Variable(SATSolver &sat_solver, VariableID variableID) : sat_solver(sat_solver), variableID(variableID), value(VariableValue::UNDECIDED)
{
}

auto SATSolver::Variable::add_clause(ClauseID clauseID)
{
    return clauses.insert(clauseID);
}

template <typename Iterator>
void SATSolver::initiate(Iterator clause_first, Iterator clause_last)
{
    unordered_map<size_t, VariableID> OriginalName2varID;
    Iterator clause_iter{clause_first};
    while (clause_iter != clause_last)
    {
        auto liter_iter = clause_iter->cbegin();
        Clause cur_clause(*this);
        ClauseID cur_clause_id = clauses.size();

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
        cur_clause.clauseID = cur_clause_id;
        clauses.push_back(cur_clause);
        clause_iter++;
    }
}

void SATSolver::update_clauses()
{
    for (auto &clause : clauses)
        clause.update();
}

bool SATSolver::solve()
{
}