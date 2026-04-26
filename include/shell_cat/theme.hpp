#pragma once
#include <string>
#include <vector>

namespace shell_cat {

enum class ColorRole {
    Default,
    Background,
    Foreground,
    Title,
    Accent,
    Success,
    Warning,
    Danger,
    Muted,
    Cat,
    Border,
    Selection
};

struct Theme {
    std::string name;
    int background = 16;
    int foreground = 252;
    int title = 117;
    int accent = 111;
    int success = 114;
    int warning = 221;
    int danger = 210;
    int muted = 244;
    int cat = 230;
    int border = 110;
    int selection = 60;
};

const Theme& default_theme();
const Theme& theme_by_name(const std::string& name);
const std::vector<Theme>& all_themes();
bool is_theme_name_valid(const std::string& name);
const std::string& default_theme_name();

} // namespace shell_cat
