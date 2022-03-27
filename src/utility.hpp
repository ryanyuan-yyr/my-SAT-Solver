#include <utility>
#include <optional>

bool exclusive_or(bool lhs, bool rhs);

std::optional<bool> variableValue2optional(int variableValue);

int optional2variableValue(std::optional<bool> value);