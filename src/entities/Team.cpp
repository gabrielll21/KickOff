#include "Team.h"

#include "Player.h"

#include <stdexcept>
#include <utility>

Team::Team(TeamId id, std::string name)
        : id_(id),
          name_(std::move(name)),
          score_(0),
          players_{},
          playerCount_(0) {
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

std::size_t Team::getPlayerCount() const {
    return playerCount_;
}

const std::array<Player*, Team::MAX_PLAYERS>& Team::getPlayers() const {
    return players_;
}

void Team::addGoal() {
    ++score_;
}

void Team::resetScore() {
    score_ = 0;
}

void Team::addPlayer(Player& player) {
    if (player.getTeam() != id_) {
        throw std::invalid_argument("Player belongs to another team");
    }
    if (playerCount_ >= MAX_PLAYERS) {
        throw std::length_error("Team already has the maximum number of players");
    }
    for (std::size_t index = 0; index < playerCount_; ++index) {
        if (players_[index] == &player) {
            throw std::invalid_argument("Player is already part of the team");
        }
    }

    players_[playerCount_] = &player;
    ++playerCount_;
}
