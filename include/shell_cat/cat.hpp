#pragma once
#include <string>
#include <vector>
#include <cstdlib>

namespace shell_cat {

enum class CatState {
    Standing,
    Sitting,
    Crouching,
    Sleeping,
    Happy,
    Eating,
    Playful,
    Grumpy
};

class Cat {
public:
    Cat();

    void update(float dt);
    void pet();
    void feed();
    void random_action();
    void refresh_mood(int hunger, int mood, int energy, int cleanliness);

    CatState state() const { return state_; }
    std::string state_name() const;
    std::string note() const;

    const std::vector<std::string>& current_sprite() const;

private:
    CatState state_ = CatState::Standing;
    float state_timer_ = 0.0f;
    float frame_timer_ = 0.0f;
    int frame_index_ = 0;

    void transition_to(CatState next);
    void pick_next_state();
    float state_duration(CatState s) const;
    int frame_count(CatState s) const;
};

} // namespace shell_cat
