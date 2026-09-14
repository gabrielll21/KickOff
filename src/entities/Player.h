#ifndef KICKOFF_PLAYER_H
#define KICKOFF_PLAYER_H

enum class Team {
    Blue,
    Red
};

class Player {
public:
    Player(float x, float y, float speed, float radius, Team team);

    float getX() const;
    float getY() const;
    float getSpeed() const;
    float getRadius() const;
    Team getTeam() const;

    void setX(float x);
    void setY(float y);
    void setSpeed(float speed);
    void setRadius(float radius);
    void setTeam(Team team);

private:
    float x_;
    float y_;
    float speed_;
    float radius_;
    Team team_;
};

#endif
