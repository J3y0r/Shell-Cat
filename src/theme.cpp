#include "shell_cat/theme.hpp"

namespace shell_cat {

namespace {

const std::vector<Theme> kThemes = {
    {"default", 16, 252, 117, 111, 114, 221, 210, 244, 230, 110, 60},
    {"solarized-dark", 234, 252, 109, 116, 108, 136, 167, 244, 230, 66, 24},
    {"solarized-light", 230, 238, 31, 37, 64, 130, 160, 245, 238, 101, 152},
    {"gruvbox-dark", 235, 223, 214, 142, 142, 214, 167, 245, 229, 172, 239},
    {"gruvbox-light", 230, 237, 130, 166, 106, 172, 124, 245, 239, 137, 180},
    {"nord", 235, 252, 81, 110, 108, 179, 174, 245, 223, 67, 60},
    {"dracula", 235, 255, 141, 117, 84, 228, 211, 246, 228, 141, 60},
    {"tokyo-night", 235, 252, 111, 117, 150, 221, 211, 245, 230, 110, 60},
    {"onedark", 236, 252, 75, 110, 114, 180, 204, 244, 229, 67, 24},
    {"monokai", 234, 252, 154, 81, 118, 221, 197, 245, 229, 110, 60},
    {"catppuccin-latte", 254, 238, 32, 68, 71, 179, 167, 245, 238, 110, 153},
    {"catppuccin-mocha", 235, 252, 111, 153, 151, 221, 211, 245, 224, 110, 60}
};

}

const Theme& default_theme() {
    return kThemes[7];
}

const std::vector<Theme>& all_themes() {
    return kThemes;
}

bool is_theme_name_valid(const std::string& name) {
    for (const Theme& theme : kThemes) {
        if (theme.name == name) {
            return true;
        }
    }
    return false;
}

const std::string& default_theme_name() {
    return default_theme().name;
}

const Theme& theme_by_name(const std::string& name) {
    for (const Theme& theme : kThemes) {
        if (theme.name == name) {
            return theme;
        }
    }
    return default_theme();
}

} // namespace shell_cat
