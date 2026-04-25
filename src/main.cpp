#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

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

using namespace shell_cat;

namespace {

enum class GameMode {
    Normal,
    Rename,
    Fishing
};

struct FishingGame {
    bool active = false;
    int lane = 1;
    int hook_lane = 1;
    float fish_x = 0.0f;
    int score = 0;
    int combo = 0;
    int best_combo = 0;
    int misses = 0;
    float time_left = 0.0f;
    float fish_speed = 20.0f;
    float catch_flash = 0.0f;
    std::string message;
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

void append_input_char(std::string& input, char key) {
    if (key == 8 || key == 127) {
        if (!input.empty()) {
            input.pop_back();
        }
        return;
    }

    if (key >= 32 && key <= 126 && input.size() < 16) {
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

        r.draw_text(2, prompt_y - 2, title);
        r.draw_text(2, prompt_y - 1, "Only letters, numbers, spaces, _ and - are allowed.");
        r.draw_text(2, prompt_y, "> " + input);

        if (!error.empty() && prompt_y + 2 < h) {
            r.draw_text(2, prompt_y + 2, error);
        }

        if (h > 2) {
            std::string hint = allow_cancel
                ? "[Enter] Confirm  [Backspace] Delete  [Esc] Cancel"
                : "[Enter] Confirm  [Backspace] Delete";
            if (static_cast<int>(hint.size()) > w - 4) {
                hint = hint.substr(0, std::max(0, w - 4));
            }
            r.draw_text(2, h - 2, hint);
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

        append_input_char(input, key);
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

void draw_normal_ui(Renderer& r, const Cat& cat, const ProfileManager& pm, const std::string& message) {
    int w = r.width();
    int h = r.height();

    if (w < 70 || h < 18) {
        r.draw_text(2, h / 2, "Terminal too small. Please resize to at least 70x18.");
        return;
    }

    const Profile& p = pm.data();
    int panel_x = std::max(28, w - 34);
    int panel_w = w - panel_x - 1;

    r.draw_text(2, 1, "Shell Cat Adventures");
    r.draw_text(2, 2, "Keep the cat fed, clean, rested, and entertained.");
    r.draw_sprite(4, 5, cat.current_sprite());
    r.draw_text(4, 10, "State: " + cat.state_name());
    r.draw_text(4, 11, "Mood:  " + cat.note());
    r.draw_text(4, 13, "Goal: " + stat_summary(p));
    if (!message.empty()) {
        r.draw_text(4, 15, "Event: " + message);
    }

    r.draw_box(panel_x, 1, panel_w, 14);
    int line = 2;
    r.draw_text(panel_x + 2, line++, "Name: " + p.name);
    r.draw_text(panel_x + 2, line++, "Day: " + std::to_string(pm.days_since_creation()));
    r.draw_text(panel_x + 2, line++, "Level: " + std::to_string(p.level));
    r.draw_text(panel_x + 2, line++, "Coins: " + std::to_string(p.coins));
    r.draw_text(panel_x + 2, line++, "Fish: " + std::to_string(p.fish_caught));
    r.draw_text(panel_x + 2, line++, "Pets/Food: " + std::to_string(p.pet_count) + "/" + std::to_string(p.feed_count));
    r.draw_text(panel_x + 2, line++, "Hunger " + bar(100 - p.hunger, 10));
    r.draw_text(panel_x + 2, line++, "Mood   " + bar(p.mood, 10));
    r.draw_text(panel_x + 2, line++, "Energy " + bar(p.energy, 10));
    r.draw_text(panel_x + 2, line++, "Clean  " + bar(p.cleanliness, 10));
    r.draw_text(panel_x + 2, line++, "Best Combo: " + std::to_string(p.best_combo));

    if (h > 2) {
        std::string hint = "[P]Pet [F]Feed [B]Brush [S]Sleep [G]Fish [N]Rename [R]Random [Q]Quit";
        if (static_cast<int>(hint.size()) > w - 4) {
            hint = hint.substr(0, w - 4);
        }
        r.draw_text(2, h - 2, hint);
    }
}

void start_fishing_game(FishingGame& game, const Renderer& r, const Profile& profile) {
    game.active = true;
    game.lane = rand() % 3;
    game.hook_lane = 1;
    game.fish_x = static_cast<float>(std::max(12, r.width() - 8));
    game.score = 0;
    game.combo = 0;
    game.best_combo = 0;
    game.misses = 0;
    game.time_left = 16.0f;
    game.catch_flash = 0.0f;
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
    if (w < 70 || h < 18) {
        r.draw_text(2, h / 2, "Terminal too small. Please resize to at least 70x18.");
        return;
    }

    r.draw_text(2, 1, "Fishing Rush");
    r.draw_text(2, 2, "W/S switch lanes. Press Space when the fish reaches the hook.");

    int area_x = 4;
    int area_y = 5;
    int area_w = w - 8;
    int lane_gap = 3;
    r.draw_box(area_x, area_y, area_w, 11);

    for (int i = 0; i < 3; ++i) {
        int y = area_y + 2 + i * lane_gap;
        r.draw_text(area_x + 2, y, std::string(area_w - 4, '.'));
        if (game.hook_lane == i) {
            r.draw_text(area_x + 3, y, "\\o/");
        }
        if (game.lane == i) {
            r.draw_text(area_x + static_cast<int>(game.fish_x), y, "<><");
        }
    }

    r.draw_text(area_x + 2, area_y + 10, "Time: " + std::to_string(static_cast<int>(game.time_left + 0.99f)) +
        "  Score: " + std::to_string(game.score) +
        "  Combo: " + std::to_string(game.combo) +
        "  Misses: " + std::to_string(game.misses));
    r.draw_text(area_x + 2, area_y + 12, game.message);

    std::string stats = "Level " + std::to_string(profile.level) +
        "  Energy " + std::to_string(profile.energy) +
        "  Mood " + std::to_string(profile.mood) +
        "  Best Combo " + std::to_string(profile.best_combo);
    r.draw_text(2, h - 2, stats.substr(0, std::max(0, w - 4)));
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

    game.active = false;
    return "Fishing over: +" + std::to_string(earned) + " coins, " + std::to_string(game.score) + " fish.";
}

void run_rename_mode(Renderer& r, ProfileManager& pm) {
    std::string name = pm.data().name;
    if (run_name_prompt(r, "Enter new name (1-16 chars):", name, true)) {
        pm.data().name = name;
        pm.save();
    }
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
    std::string status_message = "Start by keeping the cat in a good mood.";
    float autosave_timer = 0.0f;

    while (true) {
        renderer.clear();

        if (mode == GameMode::Rename) {
            run_rename_mode(renderer, pm);
            mode = GameMode::Normal;
            status_message = "Name updated.";
            continue;
        }

        if (mode == GameMode::Fishing) {
            fishing.time_left -= dt;
            fishing.fish_x -= fishing.fish_speed * dt;
            fishing.catch_flash = std::max(0.0f, fishing.catch_flash - dt);

            if (fishing.fish_x <= 4) {
                fishing.misses += 1;
                fishing.combo = 0;
                fishing.message = "Missed. Track the lane and strike later.";
                respawn_fish(fishing, renderer);
            }

            if (fishing.time_left <= 0.0f || fishing.misses >= 5) {
                status_message = resolve_fishing_rewards(fishing, pm.data());
                pm.data().last_played_at = pm.today();
                pm.save();
                mode = GameMode::Normal;
            } else {
                draw_fishing_ui(renderer, fishing, pm.data());
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
                            fishing.catch_flash = 0.3f;
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
            renderer.present();

            char key = read_key_nonblocking();
            if (key != 0) {
                char k = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
                if (k == 'q') break;
                if (k == 'p') {
                    cat.pet();
                    pm.data().pet_count += 1;
                    pm.data().mood = clampi(pm.data().mood + 8, 0, 100);
                    pm.data().energy = clampi(pm.data().energy - 1, 0, 100);
                    status_message = "The cat starts purring.";
                }
                if (k == 'f') {
                    cat.feed();
                    pm.data().feed_count += 1;
                    pm.data().hunger = clampi(pm.data().hunger - 18, 0, 100);
                    pm.data().mood = clampi(pm.data().mood + 4, 0, 100);
                    status_message = "Dinner served. Hunger drops.";
                }
                if (k == 'b') {
                    pm.data().cleanliness = clampi(pm.data().cleanliness + 20, 0, 100);
                    pm.data().mood = clampi(pm.data().mood + 5, 0, 100);
                    status_message = "Brushed and fluffy.";
                }
                if (k == 's') {
                    pm.data().energy = clampi(pm.data().energy + 25, 0, 100);
                    pm.data().mood = clampi(pm.data().mood + 2, 0, 100);
                    status_message = "A quick nap restores energy.";
                }
                if (k == 'g') {
                    if (pm.data().energy < 15) {
                        status_message = "Too tired to play. Let the cat rest first.";
                    } else {
                        start_fishing_game(fishing, renderer, pm.data());
                        mode = GameMode::Fishing;
                        status_message.clear();
                    }
                }
                if (k == 'n') {
                    mode = GameMode::Rename;
                }
                if (k == 'r') {
                    cat.random_action();
                    status_message = "The cat tries something unexpected.";
                }

                pm.data().last_played_at = pm.today();
                maybe_level_up(pm.data());
                pm.save();
            }
        }

        autosave_timer += dt;
        if (autosave_timer >= 10.0f) {
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
