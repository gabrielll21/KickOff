#include "Team.h"

#include <utility>

Team::Team(TeamId id, std::string name)
        : id_(id),
          name_(std::move(name)),
          score_(0),
          players_{} {
}

TeamId Team::getId() const {
    return id_;
}

const std::string& Team::getName() const {
    return name_;
}

int Team::getScore() const {
    return score_;
}

void Team::addGoal() {
    ++score_;
}

void Team::resetScore() {
    score_ = 0;
}
