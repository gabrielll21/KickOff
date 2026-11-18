#ifndef KICKOFF_BALL_H
#define KICKOFF_BALL_H

class Ball {
public:
    Ball(float x, float y, float velocityX, float velocityY, float radius);

    // Returns true only when a boundary reverses the ball velocity.
    bool update(float deltaTime, float left, float right,
                float bottom, float top, float goalDepth,
                float goalHalfHeight);

    float getX() const;
    float getY() const;
    float getVelocityX() const;
    float getVelocityY() const;
    float getRadius() const;

    void setX(float x);
    void setY(float y);
    void setVelocityX(float velocityX);
    void setVelocityY(float velocityY);
    void setRadius(float radius);

private:
    float x_;
    float y_;
    float velocityX_;
    float velocityY_;
    float radius_;
};

#endif
