#pragma once
#include <string>

namespace shell_cat {

struct Profile {
    std::string name;
    std::string created_at;
    int feed_count = 0;
    int pet_count = 0;
    int hunger = 20;
    int mood = 70;
    int energy = 70;
    int cleanliness = 70;
    int coins = 0;
    int level = 1;
    int best_combo = 0;
    int fish_caught = 0;
    std::string last_played_at;
};

class ProfileManager {
public:
    static constexpr const char* DEFAULT_PATH = ".shell_cat_profile";

    ProfileManager(const std::string& path = DEFAULT_PATH);

    bool load();
    bool save() const;
    bool exists() const;

    Profile& data() { return data_; }
    const Profile& data() const { return data_; }

    int days_since_creation() const;
    std::string today() const;

private:
    std::string path_;
    Profile data_;
};

} // namespace shell_cat
