#include "Ball.h"

Ball::Ball(float x, float y, float velocityX, float velocityY, float radius)
        : x_(x),
          y_(y),
          velocityX_(velocityX),
          velocityY_(velocityY),
          radius_(radius) {
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
