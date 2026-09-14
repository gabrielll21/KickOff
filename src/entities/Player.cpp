#include "Player.h"

Player::Player(float x, float y, float speed, float radius, Team team)
        : x_(x), y_(y), speed_(speed), radius_(radius), team_(team) {
}

float Player::getX() const {
    return x_;
}

float Player::getY() const {
    return y_;
}

float Player::getSpeed() const {
    return speed_;
}

float Player::getRadius() const {
    return radius_;
}

Team Player::getTeam() const {
    return team_;
}

void Player::setX(float x) {
    x_ = x;
}

void Player::setY(float y) {
    y_ = y;
}

void Player::setSpeed(float speed) {
    speed_ = speed;
}

void Player::setRadius(float radius) {
    radius_ = radius;
}

void Player::setTeam(Team team) {
    team_ = team;
}
