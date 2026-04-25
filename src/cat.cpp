#include "shell_cat/cat.hpp"
#include <cmath>

namespace shell_cat {

namespace {
    const std::vector<std::string> standing_0 = {
        " /\\_/\\",
        "( o.o )",
        " > ^ <"
    };
    const std::vector<std::string> standing_1 = {
        " /\\_/\\",
        "( -.- )",
        " > ^ <"
    };

    const std::vector<std::string> sitting_0 = {
        " /\\_/\\",
        "( o.o )",
        " /   \\"
    };
    const std::vector<std::string> sitting_1 = {
        " /\\_/\\",
        "( -.- )",
        " /   \\"
    };

    const std::vector<std::string> crouching_0 = {
        "  /\\__/",
        " ( o.o )__",
        " /       \\"
    };
    const std::vector<std::string> crouching_1 = {
        "  /\\__/",
        " ( -.- )__",
        " /       \\"
    };

    const std::vector<std::string> sleeping_0 = {
        "  |\\_/|",
        " ( -.- )",
        "  /   \\"
    };
    const std::vector<std::string> sleeping_1 = {
        "  |\\_/|",
        " ( -.- ) z",
        "  /   \\"
    };

    const std::vector<std::string> happy_0 = {
        " /\\_/\\",
        "( ^.^ )",
        " > ^ <"
    };

    const std::vector<std::string> playful_0 = {
        " /\\_/\\  o",
        "( o.o )/",
        " > ^ <"
    };
    const std::vector<std::string> playful_1 = {
        "o  /\\_/\\",
        " \\( o.o )",
        "   > ^ <"
    };

    const std::vector<std::string> grumpy_0 = {
        " /\\_/\\",
        "( >.< )",
        " /   \\\\"
    };
    const std::vector<std::string> grumpy_1 = {
        " /\\_/\\",
        "( -.-#)",
        " /   \\\\"
    };

    const std::vector<std::string> eating_0 = {
        " /\\_/\\",
        "( o.o )",
        " > ^ <   <><"
    };
    const std::vector<std::string> eating_1 = {
        " /\\_/\\",
        "( o.o )  <><",
        " > ^ <"
    };

    float randf(float min, float max) {
        return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
    }
}

Cat::Cat() {
    transition_to(CatState::Standing);
}

void Cat::transition_to(CatState next) {
    state_ = next;
    state_timer_ = state_duration(next);
    frame_index_ = 0;
    frame_timer_ = 0.3f;
}

float Cat::state_duration(CatState s) const {
    switch (s) {
        case CatState::Standing:  return randf(4.0f, 8.0f);
        case CatState::Sitting:   return randf(3.0f, 6.0f);
        case CatState::Crouching: return randf(2.0f, 5.0f);
        case CatState::Sleeping:  return randf(8.0f, 15.0f);
        case CatState::Happy:     return randf(3.0f, 5.0f);
        case CatState::Eating:    return randf(3.0f, 5.0f);
        case CatState::Playful:   return randf(4.0f, 6.0f);
        case CatState::Grumpy:    return randf(3.0f, 6.0f);
    }
    return 5.0f;
}

int Cat::frame_count(CatState s) const {
    switch (s) {
        case CatState::Standing:  return 2;
        case CatState::Sitting:   return 2;
        case CatState::Crouching: return 2;
        case CatState::Sleeping:  return 2;
        case CatState::Happy:     return 1;
        case CatState::Eating:    return 2;
        case CatState::Playful:   return 2;
        case CatState::Grumpy:    return 2;
    }
    return 1;
}

void Cat::pick_next_state() {
    float r = randf(0.0f, 100.0f);
    switch (state_) {
        case CatState::Standing:
            if (r < 40.0f)      transition_to(CatState::Sitting);
            else if (r < 75.0f) transition_to(CatState::Crouching);
            else if (r < 95.0f) transition_to(CatState::Sleeping);
            else                transition_to(CatState::Standing);
            break;
        case CatState::Sitting:
            if (r < 40.0f)      transition_to(CatState::Standing);
            else if (r < 70.0f) transition_to(CatState::Crouching);
            else if (r < 95.0f) transition_to(CatState::Sleeping);
            else                transition_to(CatState::Sitting);
            break;
        case CatState::Crouching:
            if (r < 45.0f)      transition_to(CatState::Standing);
            else if (r < 80.0f) transition_to(CatState::Sitting);
            else if (r < 95.0f) transition_to(CatState::Sleeping);
            else                transition_to(CatState::Crouching);
            break;
        case CatState::Sleeping:
            if (r < 60.0f)      transition_to(CatState::Standing);
            else if (r < 85.0f) transition_to(CatState::Sitting);
            else if (r < 95.0f) transition_to(CatState::Crouching);
            else                transition_to(CatState::Sleeping);
            break;
        case CatState::Playful:
            if (r < 55.0f)      transition_to(CatState::Standing);
            else if (r < 80.0f) transition_to(CatState::Crouching);
            else                transition_to(CatState::Sitting);
            break;
        case CatState::Grumpy:
            if (r < 50.0f)      transition_to(CatState::Sitting);
            else if (r < 75.0f) transition_to(CatState::Standing);
            else                transition_to(CatState::Sleeping);
            break;
        default:
            transition_to(CatState::Standing);
            break;
    }
}

void Cat::update(float dt) {
    state_timer_ -= dt;
    frame_timer_ -= dt;

    if (frame_timer_ <= 0.0f) {
        int fc = frame_count(state_);
        frame_index_ = (frame_index_ + 1) % fc;
        frame_timer_ = 0.5f;
    }

    if (state_timer_ <= 0.0f) {
        if (state_ == CatState::Happy || state_ == CatState::Eating) {
            transition_to(CatState::Standing);
        } else {
            pick_next_state();
        }
    }
}

void Cat::pet() {
    transition_to(CatState::Happy);
}

void Cat::feed() {
    transition_to(CatState::Eating);
}

void Cat::random_action() {
    pick_next_state();
}

void Cat::refresh_mood(int hunger, int mood, int energy, int cleanliness) {
    if (state_ == CatState::Happy || state_ == CatState::Eating) {
        return;
    }

    if (hunger >= 80 || cleanliness <= 20 || mood <= 20) {
        if (state_ != CatState::Grumpy) {
            transition_to(CatState::Grumpy);
        }
        return;
    }

    if (energy >= 80 && mood >= 75 && hunger <= 40) {
        if (state_ != CatState::Playful) {
            transition_to(CatState::Playful);
        }
        return;
    }

    if (energy <= 15) {
        if (state_ != CatState::Sleeping) {
            transition_to(CatState::Sleeping);
        }
        return;
    }
}

std::string Cat::state_name() const {
    switch (state_) {
        case CatState::Standing:  return "Standing";
        case CatState::Sitting:   return "Sitting";
        case CatState::Crouching: return "Crouching";
        case CatState::Sleeping:  return "Sleeping";
        case CatState::Happy:     return "Happy";
        case CatState::Eating:    return "Eating";
        case CatState::Playful:   return "Playful";
        case CatState::Grumpy:    return "Grumpy";
    }
    return "Unknown";
}

std::string Cat::note() const {
    switch (state_) {
        case CatState::Standing:  return "looking around";
        case CatState::Sitting:   return "resting";
        case CatState::Crouching: return "ready to pounce";
        case CatState::Sleeping:  return "zzz...";
        case CatState::Happy:     return "purr purr";
        case CatState::Eating:    return "yummy!";
        case CatState::Playful:   return "wants to chase something";
        case CatState::Grumpy:    return "needs attention";
    }
    return "";
}

const std::vector<std::string>& Cat::current_sprite() const {
    switch (state_) {
        case CatState::Standing:
            return (frame_index_ == 0) ? standing_0 : standing_1;
        case CatState::Sitting:
            return (frame_index_ == 0) ? sitting_0 : sitting_1;
        case CatState::Crouching:
            return (frame_index_ == 0) ? crouching_0 : crouching_1;
        case CatState::Sleeping:
            return (frame_index_ == 0) ? sleeping_0 : sleeping_1;
        case CatState::Happy:
            return happy_0;
        case CatState::Eating:
            return (frame_index_ == 0) ? eating_0 : eating_1;
        case CatState::Playful:
            return (frame_index_ == 0) ? playful_0 : playful_1;
        case CatState::Grumpy:
            return (frame_index_ == 0) ? grumpy_0 : grumpy_1;
    }
    return standing_0;
}

} // namespace shell_cat
