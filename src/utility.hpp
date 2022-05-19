#include <utility>
#include <optional>
#include <string>
#include <chrono>

#define claim(X)                                \
    {                                           \
        if (!(X))                               \
        {                                       \
            std::cerr << "Assert fail" << endl; \
            std::abort();                       \
        }                                       \
    }

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