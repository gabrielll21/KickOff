#include "Ball.h"

#include <cmath>

Ball::Ball(float x, float y, float velocityX, float velocityY, float radius)
        : x_(x),
          y_(y),
          velocityX_(velocityX),
          velocityY_(velocityY),
          radius_(radius) {
}

void Ball::update(float deltaTime, float left, float right,
                  float bottom, float top, float goalDepth,
                  float goalHalfHeight) {
    x_ += velocityX_ * deltaTime;
    y_ += velocityY_ * deltaTime;

    const bool insideGoalOpening =
            y_ - radius_ >= -goalHalfHeight
            && y_ + radius_ <= goalHalfHeight;
    const float minX = (insideGoalOpening ? left - goalDepth : left) + radius_;
    const float maxX = (insideGoalOpening ? right + goalDepth : right) - radius_;
    const float minY = bottom + radius_;
    const float maxY = top - radius_;

    if (x_ <= minX) {
        x_ = minX;
        velocityX_ = std::abs(velocityX_);
    } else if (x_ >= maxX) {
        x_ = maxX;
        velocityX_ = -std::abs(velocityX_);
    }

    if (y_ <= minY) {
        y_ = minY;
        velocityY_ = std::abs(velocityY_);
    } else if (y_ >= maxY) {
        y_ = maxY;
        velocityY_ = -std::abs(velocityY_);
    }
}

float Ball::getX() const {
    return x_;
}

float Ball::getY() const {
    return y_;
}

float Ball::getVelocityX() const {
    return velocityX_;
}

float Ball::getVelocityY() const {
    return velocityY_;
}

float Ball::getRadius() const {
    return radius_;
}

void Ball::setX(float x) {
    x_ = x;
}

void Ball::setY(float y) {
    y_ = y;
}

void Ball::setVelocityX(float velocityX) {
    velocityX_ = velocityX;
}

void Ball::setVelocityY(float velocityY) {
    velocityY_ = velocityY;
}

void Ball::setRadius(float radius) {
    radius_ = radius;
}
