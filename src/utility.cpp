#include "utility.hpp"
bool exclusive_or(bool lhs, bool rhs)
{
    return (lhs && !rhs) || (!lhs && rhs);
}

std::optional<bool> variableValue2optional(int variableValue)
{
    if (variableValue == 0)
        return false;
    else if (variableValue == 1)
        return true;
    else
        return std::nullopt;
}

int optional2variableValue(std::optional<bool> value)
{
    if (value.has_value())
        return static_cast<int>(value.value());
    else
        return 3;
}