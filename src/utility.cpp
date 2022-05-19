#include "utility.hpp"
bool exclusive_or(bool lhs, bool rhs)
{
    return (lhs && !rhs) || (!lhs && rhs);
}

std::optional<bool> variableValue2optional(VariableValue variableValue)
{
    if (variableValue == 0)
        return false;
    else if (variableValue == 1)
        return true;
    else
        return std::nullopt;
}

VariableValue optional2variableValue(std::optional<bool> value)
{
    if (value.has_value())
        return value.value() ? TRUE : FALSE;
    else
        return UNASSIGNED;
}

VariableValue bool2variableValue(bool value)
{
    return static_cast<VariableValue>(value);
}