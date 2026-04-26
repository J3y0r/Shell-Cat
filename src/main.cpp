#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "shell_cat/cat.hpp"
#include "shell_cat/profile.hpp"
#include "shell_cat/renderer.hpp"
#include "shell_cat/theme.hpp"

using namespace shell_cat;

namespace {

enum class GameMode {
    Normal,
    Rename,
    Fishing,
    Command,
    Settings
};

enum class SettingItemType {
    Subpage,
    Choice,
    Toggle,
    Action
};

struct FishingGame {
    int lane = 1;
    int hook_lane = 1;
    float fish_x = 0.0f;
    int score = 0;
    int combo = 0;
    int best_combo = 0;
    int misses = 0;
    float fish_speed = 20.0f;
    std::string message;
};

struct CommandState {
    std::string input;
    std::string hint = "Type /settings, /theme, /theme set <name>, /rename, /help";
};

struct SettingsState {
    std::vector<std::string> page_stack = {"root"};
    std::vector<int> selected_stack = {0};
};

struct SettingItem {
    std::string id;
    std::string label;
    SettingItemType type;
    std::string target;
};

std::string trim(const std::string& s) {
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

int clampi(int value, int low, int high) {
    return std::max(low, std::min(high, value));
}

bool is_valid_name(const std::string& name) {
    if (name.empty() || name.size() > 16) return false;
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) &&
            c != ' ' && c != '_' && c != '-') return false;
    }
    return true;
}

void set_nonblocking_input() {
#ifdef _WIN32
#else
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
#endif
}

void restore_input() {
#ifdef _WIN32
#else
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif
}

char read_key_nonblocking() {
#ifdef _WIN32
    if (_kbhit()) {
        return static_cast<char>(_getch());
    }
    return 0;
#else
    char c = 0;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return c;
    }
    return 0;
#endif
}

void append_input_char(std::string& input, char key, size_t max_len = 64) {
    if (key == 8 || key == 127) {
        if (!input.empty()) {
            input.pop_back();
        }
        return;
    }

    if (key >= 32 && key <= 126 && input.size() < max_len) {
        input.push_back(key);
    }
}

bool run_name_prompt(Renderer& r, const std::string& title, std::string& name, bool allow_cancel) {
    std::string input = name;
    std::string error;

    while (true) {
        r.clear();

        int w = r.width();
        int h = r.height();
        int prompt_y = std::max(3, h / 2);

        r.draw_text(2, prompt_y - 2, title, ColorRole::Title);
        r.draw_text(2, prompt_y - 1, "Only letters, numbers, spaces, _ and - are allowed.", ColorRole::Muted);
        r.draw_text(2, prompt_y, "> " + input, ColorRole::Accent);

        if (!error.empty() && prompt_y + 2 < h) {
            r.draw_text(2, prompt_y + 2, error, ColorRole::Danger);
        }

        if (h > 2) {
            std::string hint = allow_cancel
                ? "[Enter] Confirm  [Backspace] Delete  [Esc] Cancel"
                : "[Enter] Confirm  [Backspace] Delete";
            if (static_cast<int>(hint.size()) > w - 4) {
                hint = hint.substr(0, std::max(0, w - 4));
            }
            r.draw_text(2, h - 2, hint, ColorRole::Muted);
        }

        r.present();

        char key = read_key_nonblocking();
        if (key == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (allow_cancel && key == 27) {
            return false;
        }

        if (key == '\n' || key == '\r') {
            std::string trimmed = trim(input);
            if (is_valid_name(trimmed)) {
                name = trimmed;
                return true;
            }
            error = "Invalid name. Use 1-16 characters.";
            continue;
        }

        append_input_char(input, key, 16);
        error.clear();
    }
}

std::string bar(int value, int width) {
    int filled = clampi(value * width / 100, 0, width);
    return "[" + std::string(filled, '#') + std::string(width - filled, '.') + "]";
}

std::string stat_summary(const Profile& p) {
    if (p.hunger >= 80) return "Hungry: feed your cat soon.";
    if (p.energy <= 20) return "Sleepy: let the cat rest.";
    if (p.cleanliness <= 25) return "Messy: brush the fur.";
    if (p.mood <= 25) return "Grumpy: pet it or win a game.";
    if (p.coins < 5) return "Catch fish to build your coin stash.";
    return "Happy cycle: keep stats balanced for bonus levels.";
}

std::string bool_label(bool value) {
    return value ? "On" : "Off";
}

const std::string& current_settings_page(const SettingsState& settings) {
    return settings.page_stack.back();
}

int& current_settings_selected(SettingsState& settings) {
    return settings.selected_stack.back();
}

int current_settings_selected(const SettingsState& settings) {
    return settings.selected_stack.back();
}

const std::vector<SettingItem>& settings_page_items(const std::string& page) {
    static const std::vector<SettingItem> root = {
        {"theme", "Theme", SettingItemType::Subpage, "theme"},
        {"profile", "Profile", SettingItemType::Subpage, "profile"},
        {"display", "Display", SettingItemType::Subpage, "display"},
        {"gameplay", "Gameplay", SettingItemType::Subpage, "gameplay"},
        {"about", "About", SettingItemType::Subpage, "about"}
    };
    static const std::vector<SettingItem> theme = []() {
        std::vector<SettingItem> items;
        for (const Theme& theme : all_themes()) {
            items.push_back({"theme:" + theme.name, theme.name, SettingItemType::Choice, ""});
        }
        return items;
    }();
    static const std::vector<SettingItem> profile = {
        {"rename", "Rename Cat", SettingItemType::Action, ""}
    };
    static const std::vector<SettingItem> display = {
        {"show_hints", "Show Hints", SettingItemType::Toggle, ""},
        {"compact_ui", "Compact UI", SettingItemType::Toggle, ""}
    };
    static const std::vector<SettingItem> gameplay = {
        {"autosave_interval_seconds", "Autosave Seconds", SettingItemType::Choice, ""}
    };
    static const std::vector<SettingItem> about = {
        {"theme_count", "Theme Presets", SettingItemType::Action, ""},
        {"controls", "Controls", SettingItemType::Action, ""}
    };

    if (page == "theme") return theme;
    if (page == "profile") return profile;
    if (page == "display") return display;
    if (page == "gameplay") return gameplay;
    if (page == "about") return about;
    return root;
}

std::string settings_page_title(const std::string& page) {
    if (page == "theme") return "Settings > Theme";
    if (page == "profile") return "Settings > Profile";
    if (page == "display") return "Settings > Display";
    if (page == "gameplay") return "Settings > Gameplay";
    if (page == "about") return "Settings > About";
    return "Settings";
}

std::string setting_value(const SettingItem& item, const Profile& profile) {
    if (item.id == "show_hints") return bool_label(profile.show_hints);
    if (item.id == "compact_ui") return bool_label(profile.compact_ui);
    if (item.id == "autosave_interval_seconds") return std::to_string(profile.autosave_interval_seconds) + "s";
    if (item.id == "theme_count") return std::to_string(all_themes().size());
    if (item.id == "controls") return "Tab enter, Esc parent, / cmd";
    if (item.id.rfind("theme:", 0) == 0) {
        return profile.theme_name == item.id.substr(6) ? "Active" : "";
    }
    if (item.type == SettingItemType::Subpage) return ">";
    if (item.id == "rename") return "Open prompt";
    return "";
}

int theme_index(const std::string& name) {
    const std::vector<Theme>& themes = all_themes();
    for (size_t i = 0; i < themes.size(); ++i) {
        if (themes[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

void cycle_theme(Profile& profile, int delta) {
    const std::vector<Theme>& themes = all_themes();
    int index = theme_index(profile.theme_name);
    index = (index + delta + static_cast<int>(themes.size())) % static_cast<int>(themes.size());
    profile.theme_name = themes[index].name;
}

void cycle_autosave(Profile& profile, int delta) {
    static const int values[] = {5, 10, 15, 30, 60};
    int index = 0;
    for (int i = 0; i < 5; ++i) {
        if (values[i] == profile.autosave_interval_seconds) {
            index = i;
            break;
        }
    }
    index = (index + delta + 5) % 5;
    profile.autosave_interval_seconds = values[index];
}

void maybe_level_up(Profile& profile) {
    int target = 10 + (profile.level - 1) * 12;
    while (profile.coins >= target) {
        profile.coins -= target;
        profile.level += 1;
        profile.mood = clampi(profile.mood + 12, 0, 100);
        profile.energy = clampi(profile.energy + 8, 0, 100);
        target = 10 + (profile.level - 1) * 12;
    }
}

void decay_stats(Profile& profile, float dt) {
    static float hunger_tick = 0.0f;
    static float energy_tick = 0.0f;
    static float clean_tick = 0.0f;
    static float mood_tick = 0.0f;

    hunger_tick += dt;
    energy_tick += dt;
    clean_tick += dt;
    mood_tick += dt;

    if (hunger_tick >= 4.0f) {
        profile.hunger = clampi(profile.hunger + 1, 0, 100);
        hunger_tick = 0.0f;
    }
    if (energy_tick >= 6.0f) {
        profile.energy = clampi(profile.energy - 1, 0, 100);
        energy_tick = 0.0f;
    }
    if (clean_tick >= 9.0f) {
        profile.cleanliness = clampi(profile.cleanliness - 1, 0, 100);
        clean_tick = 0.0f;
    }
    if (mood_tick >= 5.0f) {
        int penalty = 0;
        if (profile.hunger >= 70) penalty += 1;
        if (profile.cleanliness <= 30) penalty += 1;
        if (profile.energy <= 25) penalty += 1;
        profile.mood = clampi(profile.mood - penalty, 0, 100);
        mood_tick = 0.0f;
    }
}

void draw_command_bar(Renderer& r, const CommandState& command, GameMode mode) {
    int w = r.width();
    int h = r.height();
    if (h < 3) return;

    std::string prompt = mode == GameMode::Command ? "> " + command.input : "> Press / for commands, Tab for settings";
    std::string hint = mode == GameMode::Command
        ? "[Enter] Run  [Esc] Close  [Backspace] Delete"
        : command.hint;

    if (static_cast<int>(prompt.size()) > w - 4) prompt = prompt.substr(0, std::max(0, w - 4));
    if (static_cast<int>(hint.size()) > w - 4) hint = hint.substr(0, std::max(0, w - 4));

    r.draw_box(0, h - 4, w, 4, ColorRole::Border);
    r.draw_text(2, h - 3, prompt, mode == GameMode::Command ? ColorRole::Accent : ColorRole::Muted);
    r.draw_text(2, h - 2, hint, ColorRole::Muted);
}

void draw_settings_drawer(Renderer& r, const Profile& profile, const SettingsState& settings) {
    int w = r.width();
    int h = r.height();
    if (h < 10) return;

    bool theme_page = current_settings_page(settings) == "theme";
    int drawer_h = std::min(theme_page ? 16 : 12, h - 2);
    int top = h - drawer_h;
    r.fill_rect(0, top, w, drawer_h, ' ', ColorRole::Background);
    r.draw_box(0, top, w, drawer_h, ColorRole::Border);
    r.draw_text(2, top + 1, settings_page_title(current_settings_page(settings)), ColorRole::Title);
    std::string controls = settings.page_stack.size() > 1
        ? "[W/S] Move  [A/D] Change  [Tab] Enter  [Esc] Back to parent  [Enter] Apply"
        : "[W/S] Move  [A/D] Change  [Tab] Enter  [Esc] Close  [Enter] Apply";
    r.draw_text(2, top + 2, controls, ColorRole::Muted);

    const std::vector<SettingItem>& items = settings_page_items(current_settings_page(settings));
    const int selected = current_settings_selected(settings);
    int list_width = theme_page ? std::max(24, w / 2 - 2) : w - 4;
    for (size_t i = 0; i < items.size(); ++i) {
        int y = top + 4 + static_cast<int>(i);
        if (y >= h - 1) break;
        const SettingItem& item = items[i];
        ColorRole role = static_cast<int>(i) == selected ? ColorRole::Selection : ColorRole::Foreground;
        std::string marker = static_cast<int>(i) == selected ? "> " : "  ";
        std::string line = marker + item.label;
        std::string value = setting_value(item, profile);
        if (static_cast<int>(line.size()) > list_width - 2) {
            line = line.substr(0, std::max(0, list_width - 2));
        }
        r.draw_text(2, y, line, role);
        if (!value.empty()) {
            int value_x = theme_page ? std::max(20, list_width - static_cast<int>(value.size())) : std::max(20, w - static_cast<int>(value.size()) - 4);
            r.draw_text(value_x, y, value, static_cast<int>(i) == selected ? ColorRole::Accent : ColorRole::Muted);
        }
    }

    if (theme_page && selected >= 0 && selected < static_cast<int>(all_themes().size())) {
        const Theme& preview = all_themes()[selected];
        int preview_x = std::max(28, w / 2);
        int preview_w = std::max(24, w - preview_x - 2);
        int preview_y = top + 4;
        int preview_h = std::min(9, drawer_h - 5);

        r.fill_rect_color(preview_x + 1, preview_y + 1, std::max(0, preview_w - 2), std::max(0, preview_h - 2), ' ', preview.background);
        r.draw_box_color(preview_x, preview_y, preview_w, preview_h, preview.border);
        r.draw_text_color(preview_x + 2, preview_y + 1, "Preview", preview.title);
        r.draw_text_color(preview_x + 2, preview_y + 2, preview.name, preview.accent);
        r.draw_text_color(preview_x + 2, preview_y + 4, " /\\_/\\", preview.cat);
        r.draw_text_color(preview_x + 2, preview_y + 5, "( o.o )  Accent", preview.accent);
        r.draw_text_color(preview_x + 2, preview_y + 6, " > ^ <   Warning", preview.warning);
        r.draw_text_color(preview_x + 2, preview_y + 7, "Current: " + theme_by_name(profile.theme_name).name, preview.muted);
        if (preview_h > 8) {
            r.draw_text_color(preview_x + 2, preview_y + 8, "Apply with Enter or D", preview.success);
        }
    }
}

void draw_normal_ui(Renderer& r, const Cat& cat, const ProfileManager& pm, const std::string& message) {
    int w = r.width();
    int h = r.height();

    if (w < 70 || h < 20) {
        r.draw_text(2, h / 2, "Terminal too small. Please resize to at least 70x20.", ColorRole::Warning);
        return;
    }

    const Profile& p = pm.data();
    int panel_x = p.compact_ui ? std::max(24, w - 30) : std::max(28, w - 36);
    int panel_w = w - panel_x - 1;

    r.draw_text(2, 1, "Shell Cat Adventures", ColorRole::Title);
    r.draw_text(2, 2, "Keep the cat fed, clean, rested, and entertained.", ColorRole::Muted);
    r.draw_sprite(4, 5, cat.current_sprite(), ColorRole::Cat);
    r.draw_text(4, 10, "State: " + cat.state_name(), ColorRole::Accent);
    r.draw_text(4, 11, "Mood:  " + cat.note(), ColorRole::Foreground);
    r.draw_text(4, 13, "Goal: " + stat_summary(p), ColorRole::Success);
    if (!message.empty()) {
        r.draw_text(4, 15, "Event: " + message, ColorRole::Warning);
    }

    r.draw_box(panel_x, 1, panel_w, p.compact_ui ? 13 : 15, ColorRole::Border);
    int line = 2;
    r.draw_text(panel_x + 2, line++, "Name: " + p.name, ColorRole::Foreground);
    r.draw_text(panel_x + 2, line++, "Day: " + std::to_string(pm.days_since_creation()), ColorRole::Foreground);
    r.draw_text(panel_x + 2, line++, "Level: " + std::to_string(p.level), ColorRole::Accent);
    r.draw_text(panel_x + 2, line++, "Coins: " + std::to_string(p.coins), ColorRole::Warning);
    r.draw_text(panel_x + 2, line++, "Fish: " + std::to_string(p.fish_caught), ColorRole::Foreground);
    r.draw_text(panel_x + 2, line++, "Theme: " + theme_by_name(p.theme_name).name, ColorRole::Muted);
    r.draw_text(panel_x + 2, line++, "Pets/Food: " + std::to_string(p.pet_count) + "/" + std::to_string(p.feed_count), ColorRole::Foreground);
    r.draw_text(panel_x + 2, line++, "Hunger " + bar(100 - p.hunger, 10), ColorRole::Danger);
    r.draw_text(panel_x + 2, line++, "Mood   " + bar(p.mood, 10), ColorRole::Success);
    r.draw_text(panel_x + 2, line++, "Energy " + bar(p.energy, 10), ColorRole::Accent);
    r.draw_text(panel_x + 2, line++, "Clean  " + bar(p.cleanliness, 10), ColorRole::Title);
    if (!p.compact_ui) {
        r.draw_text(panel_x + 2, line++, "Best Combo: " + std::to_string(p.best_combo), ColorRole::Foreground);
        r.draw_text(panel_x + 2, line++, "Autosave: " + std::to_string(p.autosave_interval_seconds) + "s", ColorRole::Muted);
    }

    if (p.show_hints && h > 6) {
        std::string hint = "[P]Pet [F]Feed [B]Brush [S]Sleep [G]Fish [N]Rename [R]Random [/]Cmd [Tab]Settings [Q]Quit";
        if (static_cast<int>(hint.size()) > w - 4) {
            hint = hint.substr(0, std::max(0, w - 4));
        }
        r.draw_text(2, h - 6, hint, ColorRole::Muted);
    }
}

void start_fishing_game(FishingGame& game, const Renderer& r, const Profile& profile) {
    game.lane = rand() % 3;
    game.hook_lane = 1;
    game.fish_x = static_cast<float>(std::max(12, r.width() - 8));
    game.score = 0;
    game.combo = 0;
    game.best_combo = 0;
    game.misses = 0;
    game.fish_speed = 18.0f + static_cast<float>(profile.level * 2);
    game.message = "Catch fish in the matching lane. W/S move, Space catches.";
}

void respawn_fish(FishingGame& game, const Renderer& r) {
    game.lane = rand() % 3;
    game.fish_x = static_cast<float>(std::max(12, r.width() - 8));
}

void draw_fishing_ui(Renderer& r, const FishingGame& game, const Profile& profile) {
    int w = r.width();
    int h = r.height();
    if (w < 70 || h < 20) {
        r.draw_text(2, h / 2, "Terminal too small. Please resize to at least 70x20.", ColorRole::Warning);
        return;
    }

    r.draw_text(2, 1, "Fishing Rush", ColorRole::Title);
    r.draw_text(2, 2, "W/S switch lanes. Press Space when the fish reaches the hook.", ColorRole::Muted);

    int area_x = 4;
    int area_y = 5;
    int area_w = w - 8;
    int lane_gap = 3;
    r.draw_box(area_x, area_y, area_w, 11, ColorRole::Border);

    for (int i = 0; i < 3; ++i) {
        int y = area_y + 2 + i * lane_gap;
        r.draw_text(area_x + 2, y, std::string(area_w - 4, '.'), ColorRole::Muted);
        if (game.hook_lane == i) {
            r.draw_text(area_x + 3, y, "\\o/", ColorRole::Accent);
        }
        if (game.lane == i) {
            r.draw_text(area_x + static_cast<int>(game.fish_x), y, "<><", ColorRole::Cat);
        }
    }

    r.draw_text(area_x + 2, area_y + 10, "Score: " + std::to_string(game.score) +
        "  Combo: " + std::to_string(game.combo) +
        "  Misses: " + std::to_string(game.misses), ColorRole::Foreground);
    r.draw_text(area_x + 2, area_y + 12, game.message, ColorRole::Warning);

    std::string stats = "Level " + std::to_string(profile.level) +
        "  Energy " + std::to_string(profile.energy) +
        "  Mood " + std::to_string(profile.mood) +
        "  Theme " + theme_by_name(profile.theme_name).name;
    r.draw_text(2, h - 6, stats.substr(0, std::max(0, w - 4)), ColorRole::Muted);
}

std::string resolve_fishing_rewards(FishingGame& game, Profile& profile) {
    int earned = game.score + game.best_combo;
    profile.coins += earned;
    profile.fish_caught += game.score;
    profile.best_combo = std::max(profile.best_combo, game.best_combo);
    profile.mood = clampi(profile.mood + game.score * 3 - game.misses * 2, 0, 100);
    profile.energy = clampi(profile.energy - 12 - game.misses * 2, 0, 100);
    profile.cleanliness = clampi(profile.cleanliness - 6, 0, 100);
    profile.hunger = clampi(profile.hunger + 8, 0, 100);
    maybe_level_up(profile);

    return "Fishing over: +" + std::to_string(earned) + " coins, " + std::to_string(game.score) + " fish.";
}

void run_rename_mode(Renderer& r, ProfileManager& pm) {
    std::string name = pm.data().name;
    if (run_name_prompt(r, "Enter new name (1-16 chars):", name, true)) {
        pm.data().name = name;
        pm.save();
    }
}

void apply_setting_action(const SettingItem& item, GameMode& mode, std::string& status_message) {
    if (item.id == "rename") {
        mode = GameMode::Rename;
        status_message = "Opening rename prompt.";
    } else if (item.id == "theme_count") {
        status_message = "Loaded " + std::to_string(all_themes().size()) + " theme presets.";
    } else if (item.id == "controls") {
        status_message = "Use Tab to enter pages, A/D to change values, Esc to return to parent.";
    }
}

void apply_setting_change(const SettingItem& item, int delta, ProfileManager& pm, GameMode& mode, std::string& status_message) {
    Profile& profile = pm.data();
    if (item.id.rfind("theme:", 0) == 0) {
        profile.theme_name = item.id.substr(6);
        status_message = "Theme changed to " + profile.theme_name + ".";
        pm.save();
    } else if (item.id == "show_hints") {
        profile.show_hints = !profile.show_hints;
        status_message = std::string("Show hints ") + (profile.show_hints ? "enabled." : "disabled.");
        pm.save();
    } else if (item.id == "compact_ui") {
        profile.compact_ui = !profile.compact_ui;
        status_message = std::string("Compact UI ") + (profile.compact_ui ? "enabled." : "disabled.");
        pm.save();
    } else if (item.id == "autosave_interval_seconds") {
        cycle_autosave(profile, delta == 0 ? 1 : delta);
        status_message = "Autosave set to " + std::to_string(profile.autosave_interval_seconds) + " seconds.";
        pm.save();
    } else if (item.type == SettingItemType::Action) {
        apply_setting_action(item, mode, status_message);
    }
}

void open_settings_root(SettingsState& settings) {
    settings.page_stack.assign(1, "root");
    settings.selected_stack.assign(1, 0);
}

void open_settings_page(SettingsState& settings, const std::string& page) {
    if (settings.page_stack.empty()) {
        open_settings_root(settings);
    }
    if (current_settings_page(settings) == page) {
        return;
    }
    settings.page_stack.push_back(page);
    settings.selected_stack.push_back(0);
}

bool pop_settings_page(SettingsState& settings) {
    if (settings.page_stack.size() <= 1) {
        return false;
    }
    settings.page_stack.pop_back();
    settings.selected_stack.pop_back();
    return true;
}

void open_theme_settings_page(SettingsState& settings, const Profile& profile) {
    open_settings_page(settings, "theme");
    current_settings_selected(settings) = theme_index(profile.theme_name);
}

void execute_command(const std::string& raw, ProfileManager& pm, GameMode& mode, SettingsState& settings, std::string& status_message) {
    std::string command = trim(raw);
    if (command.empty()) {
        status_message = "Empty command.";
        mode = GameMode::Normal;
        return;
    }

    if (command == "/settings") {
        open_settings_root(settings);
        mode = GameMode::Settings;
        status_message = "Settings opened.";
        return;
    }
    if (command == "/theme") {
        open_settings_root(settings);
        open_theme_settings_page(settings, pm.data());
        mode = GameMode::Settings;
        status_message = "Theme settings opened.";
        return;
    }
    if (command == "/theme next") {
        cycle_theme(pm.data(), 1);
        pm.save();
        status_message = "Theme changed to " + pm.data().theme_name + ".";
        mode = GameMode::Normal;
        return;
    }
    if (command.rfind("/theme set ", 0) == 0) {
        std::string name = trim(command.substr(11));
        if (is_theme_name_valid(name)) {
            pm.data().theme_name = name;
            pm.save();
            status_message = "Theme changed to " + name + ".";
        } else {
            status_message = "Unknown theme: " + name + ".";
        }
        mode = GameMode::Normal;
        return;
    }
    if (command == "/rename") {
        mode = GameMode::Rename;
        status_message = "Opening rename prompt.";
        return;
    }
    if (command == "/help") {
        status_message = "Commands: /settings, /theme, /theme next, /theme set <name>, /rename. Esc returns to parent page.";
        mode = GameMode::Normal;
        return;
    }

    status_message = "Unknown command.";
    mode = GameMode::Normal;
}

} // namespace

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    ProfileManager pm;
    bool first_time = false;

    if (!pm.exists()) {
        first_time = true;
    } else {
        pm.load();
    }

    Renderer renderer;
    renderer.set_theme(theme_by_name(pm.data().theme_name));
    Cat cat;

    set_nonblocking_input();

    if (first_time) {
        std::string name;
        run_name_prompt(renderer, "A new kitten appeared! Name your cat (1-16 chars):", name, false);

        pm.data().name = name;
        pm.data().created_at = pm.today();
        pm.data().last_played_at = pm.today();
        pm.save();
    }

    constexpr float target_fps = 10.0f;
    constexpr float dt = 1.0f / target_fps;

    GameMode mode = GameMode::Normal;
    FishingGame fishing;
    CommandState command;
    SettingsState settings;
    std::string status_message = "Start by keeping the cat in a good mood.";
    float autosave_timer = 0.0f;

    while (true) {
        renderer.set_theme(theme_by_name(pm.data().theme_name));
        renderer.clear();

        if (mode == GameMode::Rename) {
            run_rename_mode(renderer, pm);
            mode = GameMode::Normal;
            status_message = "Name updated.";
            continue;
        }

        if (mode == GameMode::Fishing) {
            fishing.fish_x -= fishing.fish_speed * dt;

            if (fishing.fish_x <= 4) {
                fishing.misses += 1;
                fishing.combo = 0;
                fishing.message = "Missed. Track the lane and strike later.";
                respawn_fish(fishing, renderer);
            }

            if (fishing.misses >= 5) {
                status_message = resolve_fishing_rewards(fishing, pm.data());
                pm.data().last_played_at = pm.today();
                pm.save();
                mode = GameMode::Normal;
            } else {
                draw_fishing_ui(renderer, fishing, pm.data());
                draw_command_bar(renderer, command, GameMode::Normal);
                renderer.present();

                char key = read_key_nonblocking();
                if (key != 0) {
                    char k = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
                    if (k == 'q') {
                        status_message = resolve_fishing_rewards(fishing, pm.data());
                        pm.data().last_played_at = pm.today();
                        pm.save();
                        mode = GameMode::Normal;
                    } else if (k == 'w') {
                        fishing.hook_lane = clampi(fishing.hook_lane - 1, 0, 2);
                    } else if (k == 's') {
                        fishing.hook_lane = clampi(fishing.hook_lane + 1, 0, 2);
                    } else if (key == ' ') {
                        bool lane_match = fishing.hook_lane == fishing.lane;
                        bool in_range = fishing.fish_x >= 6.0f && fishing.fish_x <= 10.5f;
                        if (lane_match && in_range) {
                            fishing.score += 1;
                            fishing.combo += 1;
                            fishing.best_combo = std::max(fishing.best_combo, fishing.combo);
                            fishing.message = "Nice catch. Combo x" + std::to_string(fishing.combo) + ".";
                            respawn_fish(fishing, renderer);
                        } else {
                            fishing.combo = 0;
                            fishing.misses += 1;
                            fishing.message = "Swing and miss. Watch the distance.";
                        }
                    }
                }
            }
        } else {
            decay_stats(pm.data(), dt);
            cat.refresh_mood(pm.data().hunger, pm.data().mood, pm.data().energy, pm.data().cleanliness);
            cat.update(dt);
            draw_normal_ui(renderer, cat, pm, status_message);
            draw_command_bar(renderer, command, mode);
            if (mode == GameMode::Settings) {
                draw_settings_drawer(renderer, pm.data(), settings);
            }
            renderer.present();

            char key = read_key_nonblocking();
            if (key != 0) {
                char k = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));

                if (mode == GameMode::Command) {
                    if (key == 27) {
                        mode = GameMode::Normal;
                    } else if (key == '\n' || key == '\r') {
                        execute_command(command.input, pm, mode, settings, status_message);
                        command.input.clear();
                    } else {
                        append_input_char(command.input, key);
                    }
                } else if (mode == GameMode::Settings) {
                    const std::vector<SettingItem>& items = settings_page_items(current_settings_page(settings));
                    if (key == 27) {
                        if (!pop_settings_page(settings)) {
                            mode = GameMode::Normal;
                        }
                    } else if (key == 9) {
                        if (!items.empty() && items[current_settings_selected(settings)].type == SettingItemType::Subpage) {
                            if (items[current_settings_selected(settings)].target == "theme") {
                                open_theme_settings_page(settings, pm.data());
                            } else {
                                open_settings_page(settings, items[current_settings_selected(settings)].target);
                            }
                        }
                    } else if (k == 'w') {
                        current_settings_selected(settings) = clampi(current_settings_selected(settings) - 1, 0, static_cast<int>(items.size()) - 1);
                    } else if (k == 's') {
                        current_settings_selected(settings) = clampi(current_settings_selected(settings) + 1, 0, static_cast<int>(items.size()) - 1);
                    } else if (k == 'a') {
                        if (!items.empty()) {
                            apply_setting_change(items[current_settings_selected(settings)], -1, pm, mode, status_message);
                        }
                    } else if (k == 'd' || key == '\n' || key == '\r') {
                        if (!items.empty()) {
                            if (items[current_settings_selected(settings)].type == SettingItemType::Subpage) {
                                if (items[current_settings_selected(settings)].target == "theme") {
                                    open_theme_settings_page(settings, pm.data());
                                } else {
                                    open_settings_page(settings, items[current_settings_selected(settings)].target);
                                }
                            } else {
                                apply_setting_change(items[current_settings_selected(settings)], 1, pm, mode, status_message);
                            }
                        }
                    }
                } else {
                    if (key == 9) {
                        open_settings_root(settings);
                        mode = GameMode::Settings;
                    } else if (key == '/') {
                        mode = GameMode::Command;
                        command.input = "/";
                    } else if (k == 'q') {
                        break;
                    } else if (k == 'p') {
                        cat.pet();
                        pm.data().pet_count += 1;
                        pm.data().mood = clampi(pm.data().mood + 8, 0, 100);
                        pm.data().energy = clampi(pm.data().energy - 1, 0, 100);
                        status_message = "The cat starts purring.";
                    } else if (k == 'f') {
                        cat.feed();
                        pm.data().feed_count += 1;
                        pm.data().hunger = clampi(pm.data().hunger - 18, 0, 100);
                        pm.data().mood = clampi(pm.data().mood + 4, 0, 100);
                        status_message = "Dinner served. Hunger drops.";
                    } else if (k == 'b') {
                        pm.data().cleanliness = clampi(pm.data().cleanliness + 20, 0, 100);
                        pm.data().mood = clampi(pm.data().mood + 5, 0, 100);
                        status_message = "Brushed and fluffy.";
                    } else if (k == 's') {
                        pm.data().energy = clampi(pm.data().energy + 25, 0, 100);
                        pm.data().mood = clampi(pm.data().mood + 2, 0, 100);
                        status_message = "A quick nap restores energy.";
                    } else if (k == 'g') {
                        if (pm.data().energy < 15) {
                            status_message = "Too tired to play. Let the cat rest first.";
                        } else {
                            start_fishing_game(fishing, renderer, pm.data());
                            mode = GameMode::Fishing;
                            status_message.clear();
                        }
                    } else if (k == 'n') {
                        mode = GameMode::Rename;
                    } else if (k == 'r') {
                        cat.random_action();
                        status_message = "The cat tries something unexpected.";
                    }

                    pm.data().last_played_at = pm.today();
                    maybe_level_up(pm.data());
                    pm.save();
                }
            }
        }

        autosave_timer += dt;
        if (autosave_timer >= static_cast<float>(pm.data().autosave_interval_seconds)) {
            pm.data().last_played_at = pm.today();
            pm.save();
            autosave_timer = 0.0f;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<int>(dt * 1000.0f))
        );
    }

    restore_input();
    return 0;
}
