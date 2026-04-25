#include "shell_cat/profile.hpp"
#include <fstream>
#include <iostream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace shell_cat {

ProfileManager::ProfileManager(const std::string& path) : path_(path) {}

bool ProfileManager::exists() const {
    std::ifstream f(path_);
    return f.good();
}

bool ProfileManager::load() {
    std::ifstream f(path_);
    if (!f.is_open()) return false;

    auto clamp = [](int value, int low, int high) {
        if (value < low) return low;
        if (value > high) return high;
        return value;
    };

    std::string line;
    while (std::getline(f, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        try {
            if (key == "name") data_.name = val;
            else if (key == "created_at") data_.created_at = val;
            else if (key == "feed_count") data_.feed_count = clamp(std::stoi(val), 0, 999999);
            else if (key == "pet_count") data_.pet_count = clamp(std::stoi(val), 0, 999999);
            else if (key == "hunger") data_.hunger = clamp(std::stoi(val), 0, 100);
            else if (key == "mood") data_.mood = clamp(std::stoi(val), 0, 100);
            else if (key == "energy") data_.energy = clamp(std::stoi(val), 0, 100);
            else if (key == "cleanliness") data_.cleanliness = clamp(std::stoi(val), 0, 100);
            else if (key == "coins") data_.coins = clamp(std::stoi(val), 0, 999999);
            else if (key == "level") data_.level = clamp(std::stoi(val), 1, 999);
            else if (key == "best_combo") data_.best_combo = clamp(std::stoi(val), 0, 999999);
            else if (key == "fish_caught") data_.fish_caught = clamp(std::stoi(val), 0, 999999);
            else if (key == "last_played_at") data_.last_played_at = val;
        } catch (...) {
        }
    }

    if (data_.level < 1) data_.level = 1;
    return true;
}

bool ProfileManager::save() const {
    std::ofstream f(path_);
    if (!f.is_open()) return false;
    f << "name=" << data_.name << "\n";
    f << "created_at=" << data_.created_at << "\n";
    f << "feed_count=" << data_.feed_count << "\n";
    f << "pet_count=" << data_.pet_count << "\n";
    f << "hunger=" << data_.hunger << "\n";
    f << "mood=" << data_.mood << "\n";
    f << "energy=" << data_.energy << "\n";
    f << "cleanliness=" << data_.cleanliness << "\n";
    f << "coins=" << data_.coins << "\n";
    f << "level=" << data_.level << "\n";
    f << "best_combo=" << data_.best_combo << "\n";
    f << "fish_caught=" << data_.fish_caught << "\n";
    f << "last_played_at=" << data_.last_played_at << "\n";
    return f.good();
}

std::string ProfileManager::today() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

int ProfileManager::days_since_creation() const {
    if (data_.created_at.empty()) return 1;

    std::tm created_tm = {};
    std::istringstream css(data_.created_at);
    css >> std::get_time(&created_tm, "%Y-%m-%d");
    if (css.fail()) return 1;

    std::tm today_tm = {};
    std::istringstream tss(today());
    tss >> std::get_time(&today_tm, "%Y-%m-%d");
    if (tss.fail()) return 1;

    auto created_time = std::mktime(&created_tm);
    auto today_time = std::mktime(&today_tm);
    if (created_time == -1 || today_time == -1) return 1;

    double diff = std::difftime(today_time, created_time);
    int days = static_cast<int>(diff / (24 * 3600)) + 1;
    return days < 1 ? 1 : days;
}

} // namespace shell_cat
