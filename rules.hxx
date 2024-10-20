#include <cstdint>

enum class Rule : uint8_t
{
    NONE = 0,
    RECURSIVE_SOLVING = 1 << 0,
    REMOVE_TRIVIAL = 1 << 1,
    REMOVE_SINGULAR = 1 << 2,
    REMOVE_PURE = 1 << 3
};

inline constexpr Rule operator|(Rule x, Rule y)
{
    return (Rule) (((uint8_t) x) | ((uint8_t) y));
}

inline constexpr Rule operator&(Rule x, Rule y)
{
    return (Rule) (((uint8_t) x) & ((uint8_t) y));
}
