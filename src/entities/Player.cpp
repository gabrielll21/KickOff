#include "Player.h"

#include <algorithm>

Player::Player(float x, float y, float speed, float radius, TeamId team)
        : x_(x), y_(y), speed_(speed), radius_(radius), team_(team) {
}

void Player::move(float directionX, float directionY, float deltaTime) {
    x_ += directionX * speed_ * deltaTime;
    y_ += directionY * speed_ * deltaTime;
}

void Player::keepInside(float left, float right, float bottom, float top) {
    x_ = std::max(left + radius_, std::min(x_, right - radius_));
    y_ = std::max(bottom + radius_, std::min(y_, top - radius_));
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

TeamId Player::getTeam() const {
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

void Player::setTeam(TeamId team) {
    team_ = team;
}
