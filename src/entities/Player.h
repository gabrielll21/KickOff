#ifndef KICKOFF_PLAYER_H
#define KICKOFF_PLAYER_H

#include "Team.h"

class Player {
public:
    Player(float x, float y, float speed, float radius, TeamId team);

    void move(float directionX, float directionY, float deltaTime);
    void keepInside(float left, float right, float bottom, float top);

    float getX() const;
    float getY() const;
    float getSpeed() const;
    float getRadius() const;
    TeamId getTeam() const;

    void setX(float x);
    void setY(float y);
    void setSpeed(float speed);
    void setRadius(float radius);
    void setTeam(TeamId team);

private:
    float x_;
    float y_;
    float speed_;
    float radius_;
    TeamId team_;
};

#endif
