#include <utility>
bool exclusive_or(bool lhs, bool rhs)
{
    return lhs && !rhs || !lhs && rhs;
}