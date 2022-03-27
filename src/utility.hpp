#include <utility>
#include <optional>

enum VariableValue
{
    FALSE,
    TRUE,
    UNDECIDED,
};

bool exclusive_or(bool lhs, bool rhs);

std::optional<bool> variableValue2optional(VariableValue variableValue);

VariableValue optional2variableValue(std::optional<bool> value);

VariableValue bool2variableValue(bool value);