#include "sat_solver.hpp"

SATSolver::Literal::Literal(SATSolver *sat_solver, VariableID variable, bool literal_type) : variableID(variable), sat_solver(sat_solver), literal_type(literal_type) {}

VariableValue SATSolver::Literal::get_value() const
{
    return get_value_if(sat_solver->get_variable(variableID).value);
}

VariableValue SATSolver::Literal::get_value_if(VariableValue variableValue) const
{
    if (variableValue == VariableValue::UNDECIDED)
        return UNDECIDED;
    else
    {
        bool b_value = variableValue == VariableValue::TRUE;
        return exclusive_or(literal_type, b_value) ? FALSE : TRUE;
    }
}

SATSolver::VariableID SATSolver::Literal::get_variable_id() const
{
    return variableID;
}

SATSolver::Variable &SATSolver::Literal::get_variable() const
{
    return sat_solver->get_variable(get_variable_id());
}

bool SATSolver::Literal::get_literal_type() const
{
    return literal_type;
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
    auto insert_res = literals.insert({variableID, Literal(&sat_solver, variableID, literal_type)});
    if (insert_res.second)
    {
        claim(literals_by_value[insert_res.first->second.get_value()].insert(variableID).second);
        return true;
    }
    else if (insert_res.first->second.get_literal_type() == literal_type)
        return true;
    else
        return false;
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
            claim(literals_by_value[literals[literalID].get_value()].insert(literalID).second);
        }
    }
}

void SATSolver::Clause::change_assignment(VariableID variableID, VariableValue from, VariableValue to)
{
    auto &literal = literals[variableID];
    claim(literals_by_value[literal.get_value_if(from)].erase(variableID) == 1);
    claim(literals_by_value[literal.get_value_if(to)].insert(variableID).second);
}

bool SATSolver::Clause::assign(VariableID variableID, bool b_variableValue)
{
    change_assignment(variableID, UNDECIDED, bool2variableValue(b_variableValue));
    if (to_decide_num() == 1)
        sat_solver.unipropagateQueue.push_back(clauseID);
    if (is_conflict())
        return false;
    else
        return true;
}

void SATSolver::Clause::reset(VariableID variableID)
{
    change_assignment(variableID, sat_solver.get_variable(variableID).value, UNDECIDED);
}

bool SATSolver::Clause::is_conflict()
{
    return literals_by_value[UNDECIDED].size() == 0 && literals_by_value[TRUE].size() == 0;
}

SATSolver::Variable::Variable(SATSolver &sat_solver, VariableID variableID) : sat_solver(sat_solver), variableID(variableID), value(VariableValue::UNDECIDED)
{
}

void SATSolver::update_clauses()
{
    for (auto &clause : clauses)
        clause.update();
}

vector<SATSolver::Index> SATSolver::ImplicationGraph::confilict_analysis(ClauseID conflict_clause)
{
    auto &init_learnt_clause = sat_solver.get_clause(conflict_clause).get_literals();
    claim(init_learnt_clause.find(stack.back().variableID) != init_learnt_clause.end());
    unordered_set<Index> other_DL_nodes;
    set<Index, std::greater<Index>> cur_DL_nodes;
    for (auto &varID_literal : init_learnt_clause)
    {
        Index node_pos = var2pos[varID_literal.first];
        if (stack[node_pos].decision_level == get_decision_level())
            cur_DL_nodes.insert(node_pos);
        else
            other_DL_nodes.insert(node_pos);
    }
    claim(!cur_DL_nodes.empty());

    while (cur_DL_nodes.size() > 1)
    {
        Index cur_node_pos = cur_DL_nodes.begin().operator*();
        cur_DL_nodes.erase(cur_DL_nodes.begin());
        auto &cur_clause_literals = sat_solver.get_clause(stack[cur_node_pos].derive_from.value()).get_literals();
        for (auto &varID_literal : cur_clause_literals)
        {
            auto var_id = varID_literal.first;
            if (var_id != stack[cur_node_pos].variableID)
            {
                if (stack[var2pos[var_id]].decision_level == get_decision_level())
                    cur_DL_nodes.insert(var2pos[var_id]);
                else
                    other_DL_nodes.insert(var2pos[var_id]);
            }
        }
    }
    claim(cur_DL_nodes.size() == 1);
    vector<Index> learnt_clause_pos;
    learnt_clause_pos.push_back(cur_DL_nodes.begin().operator*());
    copy(other_DL_nodes.begin(), other_DL_nodes.end(), back_inserter(learnt_clause_pos));
    return learnt_clause_pos;
}

optional<SATSolver::ClauseID> SATSolver::assign(VariableID variableID, bool b_variableValue)
{
    auto variableValue = bool2variableValue(b_variableValue);
    auto oldValue = get_variable(variableID).value;
    claim(oldValue == UNDECIDED);

    variables_by_value[oldValue].erase(variableID);
    variables_by_value[variableValue].insert(variableID);

    optional<ClauseID> conflict_clause;
    // For consistency, even if a conflict is detected, the assignment should be performed for all clauses.
    for (auto &clauseID : get_variable(variableID).clauses)
        if (!get_clause(clauseID).assign(variableID, b_variableValue) && !conflict_clause.has_value())
            conflict_clause = clauseID;

    get_variable(variableID).value = variableValue;

    return conflict_clause;
}

void SATSolver::reset(VariableID variableID)
{
    auto oldValue = get_variable(variableID).value;
    claim(oldValue != UNDECIDED);

    claim(variables_by_value[oldValue].erase(variableID) == 1);
    claim(variables_by_value[UNDECIDED].insert(variableID).second);

    for (auto &clauseID : get_variable(variableID).clauses)
        get_clause(clauseID).reset(variableID);

    // Assignment to variable should be after Clause::reset, since Clause::reset uses the value of variables to determine the old value.
    get_variable(variableID)
        .value = UNDECIDED;
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
        claim(!to_propagate_clause.is_conflict());

        if (to_propagate_clause.value() == TRUE)
        {
            // Nothing to do
        }
        else
        {
            claim(to_propagate_clause.get_literals_by_value(UNDECIDED).size() == 1);
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
            claim(!assign(decision.first, decision.second).has_value());
            implicationGraph.push_decision_node(decision.first);
        }

        auto unipropagate_result = unipropagate();
        if (unipropagate_result.has_value())
        {
            auto conflict_result = implicationGraph.confilict_analysis(unipropagate_result.value());
            claim(!conflict_result.empty());
            if (conflict_result == vector<Index>{0} && !implicationGraph[0].is_decision_node())
                return false;
            else
            {
                // Assert that current decision level cannot be 0;
                // Otherwise conflict_result == {0} and stack[0] is not a decision node.
                claim(implicationGraph.get_decision_level() != 0);

                log_stream << "[Conflict analysis] ";
                for (auto pos : conflict_result)
                    log_stream << VarID2originalName[implicationGraph[pos].variableID] << ", ";
                log_stream << endl;
                auto learnt_clause_id = clauses.size();
                Clause learnt_clause(*this, learnt_clause_id);
                for (auto pos : conflict_result)
                {
                    auto var_id = implicationGraph[pos].variableID;
                    claim(learnt_clause.add_literal(var_id, !get_variable(var_id).value));
                }
                learnt_clause.update();
                for (auto literal : learnt_clause.get_literals())
                {
                    get_variable(literal.first).add_clause(learnt_clause_id);
                }

                add_clause(std::move(learnt_clause));
                claim(unipropagateQueue.size() == 0);
                unipropagateQueue.push_back(learnt_clause_id);
                /**
                 * @brief Backjump
                 *
                 */
                statistic.backjumpNum++;
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