#ifndef KICKOFF_TEAM_H
#define KICKOFF_TEAM_H

#include <array>
#include <string>

class Player;

enum class TeamId {
    Blue,
    Red
};

class Team {
public:
    static constexpr std::size_t MAX_PLAYERS = 3;

    Team(TeamId id, std::string name);

    TeamId getId() const;
    const std::string& getName() const;
    int getScore() const;
    std::size_t getPlayerCount() const;
    const std::array<Player*, MAX_PLAYERS>& getPlayers() const;

    void addGoal();
    void resetScore();
    void addPlayer(Player& player);

private:
    TeamId id_;
    std::string name_;
    int score_;
    std::array<Player*, MAX_PLAYERS> players_;
    std::size_t playerCount_;
};

#endif
