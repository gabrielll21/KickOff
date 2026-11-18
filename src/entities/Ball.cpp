#include "Ball.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float BALL_ROLLING_DECELERATION = 5.0F;
}

Ball::Ball(float x, float y, float velocityX, float velocityY, float radius)
        : x_(x),
          y_(y),
          velocityX_(velocityX),
          velocityY_(velocityY),
          radius_(radius) {
}

bool Ball::update(float deltaTime, float left, float right,
                  float bottom, float top, float goalDepth,
                  float goalHalfHeight) {
    if (deltaTime > 0.0F) {
        const float speed = std::hypot(velocityX_, velocityY_);
        if (speed > 0.0F) {
            const float movingTime = std::min(deltaTime, speed / BALL_ROLLING_DECELERATION);
            const float newSpeed = deltaTime >= speed / BALL_ROLLING_DECELERATION
                    ? 0.0F : std::max(0.0F, speed - BALL_ROLLING_DECELERATION * movingTime);
            const float distance = (speed + newSpeed) * 0.5F * movingTime;
            x_ += velocityX_ / speed * distance;
            y_ += velocityY_ / speed * distance;
            if (newSpeed == 0.0F) {
                velocityX_ = 0.0F;
                velocityY_ = 0.0F;
            } else {
                velocityX_ *= newSpeed / speed;
                velocityY_ *= newSpeed / speed;
            }
        }
    }
    bool rebounded = false;

    const bool insideGoalOpening =
            y_ - radius_ >= -goalHalfHeight
            && y_ + radius_ <= goalHalfHeight;
    const float minX = (insideGoalOpening ? left - goalDepth : left) + radius_;
    const float maxX = (insideGoalOpening ? right + goalDepth : right) - radius_;
    const float minY = bottom + radius_;
    const float maxY = top - radius_;

    // Preserve the distance left after a wall hit within the existing short
    // steps. Zero-time contact corrections only clamp and never apply drag.
    if (x_ <= minX) {
        rebounded = rebounded || velocityX_ < 0.0F;
        x_ = deltaTime > 0.0F ? std::min(maxX, 2.0F * minX - x_) : minX;
        velocityX_ = std::abs(velocityX_);
    } else if (x_ >= maxX) {
        rebounded = rebounded || velocityX_ > 0.0F;
        x_ = deltaTime > 0.0F ? std::max(minX, 2.0F * maxX - x_) : maxX;
        velocityX_ = -std::abs(velocityX_);
    }

    if (y_ <= minY) {
        rebounded = rebounded || velocityY_ < 0.0F;
        y_ = deltaTime > 0.0F ? std::min(maxY, 2.0F * minY - y_) : minY;
        velocityY_ = std::abs(velocityY_);
    } else if (y_ >= maxY) {
        rebounded = rebounded || velocityY_ > 0.0F;
        y_ = deltaTime > 0.0F ? std::max(minY, 2.0F * maxY - y_) : maxY;
        velocityY_ = -std::abs(velocityY_);
    }
    return rebounded;
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
