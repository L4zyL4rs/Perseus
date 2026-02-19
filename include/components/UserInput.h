#include <array>

enum class Key{
    mouseL,
    mouseR,
    space,
    w,
    a,
    s,
    d,
    esc,
    lShift,
    lCtrl,
    LAST
};

struct UserInput {
    std::array<bool, (std::size_t)Key::LAST> keys{};
    double mouseX{};
    double mouseY{};
};
