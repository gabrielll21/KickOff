#include "entities/Ball.h"
#include "entities/Player.h"
#include "entities/Team.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string>

namespace {
constexpr double WORLD_WIDTH = 120.0;
constexpr double WORLD_HEIGHT = 80.0;
constexpr float FIELD_HALF_WIDTH = 52.5F;
constexpr float PLAYER_SPEED = 12.0F;
constexpr float PLAYER_RADIUS = 2.0F;
constexpr float BALL_INITIAL_X = 0.0F;
constexpr float BALL_INITIAL_Y = 0.0F;
constexpr float BALL_INITIAL_VELOCITY_X = 14.0F;
constexpr float BALL_INITIAL_VELOCITY_Y = 9.0F;
constexpr float BALL_RADIUS = 1.0F;
constexpr float BALL_IMPACT_SPEED = 20.0F;
constexpr float FIELD_HALF_HEIGHT = 34.0F;
constexpr float GOAL_DEPTH = 2.44F;
constexpr float GOAL_HALF_HEIGHT = 3.66F;
constexpr float GOAL_STATE_DURATION = 2.0F;
constexpr double PI = 3.14159265358979323846;

struct PlayerPosition {
    float x;
    float y;
};

struct TeamControls {
    int upKey;
    int downKey;
    int leftKey;
    int rightKey;
    int switchKey;
};

struct TeamControlState {
    std::size_t controlledPlayerIndex;
    bool switchKeyWasPressed;
};

constexpr std::array<PlayerPosition, Team::MAX_PLAYERS> BLUE_INITIAL_POSITIONS{{
    {-25.0F, 0.0F},
    {-35.0F, 12.0F},
    {-35.0F, -12.0F}
}};
constexpr std::array<PlayerPosition, Team::MAX_PLAYERS> RED_INITIAL_POSITIONS{{
    {25.0F, 0.0F},
    {35.0F, 12.0F},
    {35.0F, -12.0F}
}};
constexpr TeamControls BLUE_CONTROLS{
    GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_TAB
};
constexpr TeamControls RED_CONTROLS{
    GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_ENTER
};

enum class GoalSide { None, Left, Right };
enum class MatchState { Playing, Goal };

void configureProjection(GLFWwindow*, int framebufferWidth, int framebufferHeight) {
    if (framebufferWidth == 0 || framebufferHeight == 0) {
        return;
    }

    const double worldAspect = WORLD_WIDTH / WORLD_HEIGHT;
    const double framebufferAspect =
            static_cast<double>(framebufferWidth) / framebufferHeight;

    int viewportWidth = framebufferWidth;
    int viewportHeight = framebufferHeight;
    int viewportX = 0;
    int viewportY = 0;

    if (framebufferAspect > worldAspect) {
        viewportWidth = static_cast<int>(framebufferHeight * worldAspect);
        viewportX = (framebufferWidth - viewportWidth) / 2;
    } else {
        viewportHeight = static_cast<int>(framebufferWidth / worldAspect);
        viewportY = (framebufferHeight - viewportHeight) / 2;
    }

    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-WORLD_WIDTH / 2.0, WORLD_WIDTH / 2.0,
            -WORLD_HEIGHT / 2.0, WORLD_HEIGHT / 2.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void drawRectangle(float left, float bottom, float right, float top) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(left, bottom);
    glVertex2f(right, bottom);
    glVertex2f(right, top);
    glVertex2f(left, top);
    glEnd();
}

void drawFilledRectangle(float left, float bottom, float right, float top) {
    glBegin(GL_QUADS);
    glVertex2f(left, bottom);
    glVertex2f(right, bottom);
    glVertex2f(right, top);
    glVertex2f(left, top);
    glEnd();
}

GoalSide detectGoal(const Ball& ball) {
    const bool fitsBetweenPosts =
            ball.getY() - ball.getRadius() >= -GOAL_HALF_HEIGHT
            && ball.getY() + ball.getRadius() <= GOAL_HALF_HEIGHT;

    if (!fitsBetweenPosts) {
        return GoalSide::None;
    }
    if (ball.getX() + ball.getRadius() <= -FIELD_HALF_WIDTH
        && ball.getX() - ball.getRadius() >= -FIELD_HALF_WIDTH - GOAL_DEPTH) {
        return GoalSide::Left;
    }
    if (ball.getX() - ball.getRadius() >= FIELD_HALF_WIDTH
        && ball.getX() + ball.getRadius() <= FIELD_HALF_WIDTH + GOAL_DEPTH) {
        return GoalSide::Right;
    }
    return GoalSide::None;
}

bool updateScore(const Ball& ball, Team& leftTeam, Team& rightTeam) {
    const GoalSide goal = detectGoal(ball);
    if (goal != GoalSide::None) {
        if (goal == GoalSide::Left) {
            rightTeam.addGoal();
        } else {
            leftTeam.addGoal();
        }
        return true;
    }
    return false;
}

void drawDigit(int digit, float x, float y, float scale) {
    static const bool SEGMENTS[10][7] = {
        {true, true, true, false, true, true, true},
        {false, false, true, false, false, true, false},
        {true, false, true, true, true, false, true},
        {true, false, true, true, false, true, true},
        {false, true, true, true, false, true, false},
        {true, true, false, true, false, true, true},
        {true, true, false, true, true, true, true},
        {true, false, true, false, false, true, false},
        {true, true, true, true, true, true, true},
        {true, true, true, true, false, true, true}
    };
    const float width = 3.0F * scale;
    const float height = 5.0F * scale;
    const float thickness = 0.45F * scale;

    if (SEGMENTS[digit][0]) drawFilledRectangle(x, y + height - thickness, x + width, y + height);
    if (SEGMENTS[digit][1]) drawFilledRectangle(x, y + height / 2.0F, x + thickness, y + height);
    if (SEGMENTS[digit][2]) drawFilledRectangle(x + width - thickness, y + height / 2.0F, x + width, y + height);
    if (SEGMENTS[digit][3]) drawFilledRectangle(x, y + height / 2.0F - thickness / 2.0F, x + width, y + height / 2.0F + thickness / 2.0F);
    if (SEGMENTS[digit][4]) drawFilledRectangle(x, y, x + thickness, y + height / 2.0F);
    if (SEGMENTS[digit][5]) drawFilledRectangle(x + width - thickness, y, x + width, y + height / 2.0F);
    if (SEGMENTS[digit][6]) drawFilledRectangle(x, y, x + width, y + thickness);
}

float numberWidth(const std::string& text, float scale) {
    return 3.0F * scale * static_cast<float>(text.size())
           + 0.8F * scale * static_cast<float>(text.size() - 1U);
}

void drawNumber(int value, float left, float y, float scale) {
    const std::string text = std::to_string(value);
    float x = left;
    for (char character : text) {
        drawDigit(character - '0', x, y, scale);
        x += 3.8F * scale;
    }
}

void drawScore(const Team& leftTeam, const Team& rightTeam) {
    constexpr float Y = 35.5F;
    constexpr float SCALE = 0.75F;
    const std::string leftText = std::to_string(leftTeam.getScore());

    glColor3f(1.0F, 1.0F, 1.0F);
    drawNumber(leftTeam.getScore(), -2.5F - numberWidth(leftText, SCALE), Y, SCALE);
    drawFilledRectangle(-0.45F, Y + 2.4F, 0.45F, Y + 3.3F);
    drawFilledRectangle(-0.45F, Y + 0.8F, 0.45F, Y + 1.7F);
    drawNumber(rightTeam.getScore(), 2.5F, Y, SCALE);
}

void drawGoalMessage() {
    constexpr float X = -17.0F;
    constexpr float Y = -4.0F;
    constexpr float WIDTH = 5.0F;
    constexpr float HEIGHT = 8.0F;
    constexpr float THICKNESS = 0.8F;
    constexpr float STEP = 7.0F;

    glColor3f(1.0F, 0.85F, 0.1F);

    drawFilledRectangle(X, Y, X + THICKNESS, Y + HEIGHT);
    drawFilledRectangle(X, Y + HEIGHT - THICKNESS, X + WIDTH, Y + HEIGHT);
    drawFilledRectangle(X, Y, X + WIDTH, Y + THICKNESS);
    drawFilledRectangle(X + WIDTH - THICKNESS, Y, X + WIDTH, Y + HEIGHT / 2.0F);
    drawFilledRectangle(X + WIDTH / 2.0F, Y + HEIGHT / 2.0F - THICKNESS / 2.0F,
                        X + WIDTH, Y + HEIGHT / 2.0F + THICKNESS / 2.0F);

    const float oX = X + STEP;
    drawFilledRectangle(oX, Y, oX + THICKNESS, Y + HEIGHT);
    drawFilledRectangle(oX + WIDTH - THICKNESS, Y, oX + WIDTH, Y + HEIGHT);
    drawFilledRectangle(oX, Y + HEIGHT - THICKNESS, oX + WIDTH, Y + HEIGHT);
    drawFilledRectangle(oX, Y, oX + WIDTH, Y + THICKNESS);

    const float aX = X + 2.0F * STEP;
    drawFilledRectangle(aX, Y, aX + THICKNESS, Y + HEIGHT);
    drawFilledRectangle(aX + WIDTH - THICKNESS, Y, aX + WIDTH, Y + HEIGHT);
    drawFilledRectangle(aX, Y + HEIGHT - THICKNESS, aX + WIDTH, Y + HEIGHT);
    drawFilledRectangle(aX, Y + HEIGHT / 2.0F - THICKNESS / 2.0F,
                        aX + WIDTH, Y + HEIGHT / 2.0F + THICKNESS / 2.0F);

    const float lX = X + 3.0F * STEP;
    drawFilledRectangle(lX, Y, lX + THICKNESS, Y + HEIGHT);
    drawFilledRectangle(lX, Y, lX + WIDTH, Y + THICKNESS);

    const float exclamationX = X + 4.0F * STEP;
    drawFilledRectangle(exclamationX, Y + 2.0F,
                        exclamationX + THICKNESS, Y + HEIGHT);
    drawFilledRectangle(exclamationX, Y,
                        exclamationX + THICKNESS, Y + THICKNESS);
}

void drawCircle(float centerX, float centerY, float radius) {
    constexpr int SEGMENTS = 96;

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < SEGMENTS; ++i) {
        const double angle = 2.0 * PI * i / SEGMENTS;
        glVertex2d(centerX + radius * std::cos(angle),
                   centerY + radius * std::sin(angle));
    }
    glEnd();
}

void drawArc(float centerX, float centerY, float radius,
             double startAngle, double endAngle) {
    constexpr int SEGMENTS = 48;

    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= SEGMENTS; ++i) {
        const double angle =
                startAngle + (endAngle - startAngle) * i / SEGMENTS;
        glVertex2d(centerX + radius * std::cos(angle),
                   centerY + radius * std::sin(angle));
    }
    glEnd();
}

bool isKeyPressed(GLFWwindow* window, int key) {
    return glfwGetKey(window, key) == GLFW_PRESS;
}

std::size_t findClosestPlayerIndex(const Team& team, const Ball& ball,
                                   std::size_t currentIndex) {
    const auto& players = team.getPlayers();
    const auto distanceSquared = [&ball](const Player& player) {
        const float dx = player.getX() - ball.getX();
        const float dy = player.getY() - ball.getY();
        return dx * dx + dy * dy;
    };

    // Starting with the current player preserves selection on a nearest tie.
    std::size_t closestIndex = currentIndex;
    float closestDistanceSquared = distanceSquared(*players[currentIndex]);
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        const float candidateDistanceSquared = distanceSquared(*players[index]);
        if (candidateDistanceSquared < closestDistanceSquared) {
            closestIndex = index;
            closestDistanceSquared = candidateDistanceSquared;
        }
    }
    return closestIndex;
}

void updateControlledPlayerIndex(TeamControlState& state,
                                 const Team& team, const Ball& ball,
                                 bool switchKeyPressed) {
    if (switchKeyPressed && !state.switchKeyWasPressed
        && team.getPlayerCount() > 0U) {
        state.controlledPlayerIndex =
                findClosestPlayerIndex(team, ball, state.controlledPlayerIndex);
    }
    state.switchKeyWasPressed = switchKeyPressed;
}

Player& getControlledPlayer(Team& team, const TeamControlState& state) {
    return *team.getPlayers()[state.controlledPlayerIndex];
}

void handlePlayerInput(GLFWwindow* window, Player& player,
                       const TeamControls& controls, float deltaTime) {
    float directionX = 0.0F;
    float directionY = 0.0F;

    if (isKeyPressed(window, controls.upKey)) {
        directionY += 1.0F;
    }
    if (isKeyPressed(window, controls.downKey)) {
        directionY -= 1.0F;
    }
    if (isKeyPressed(window, controls.leftKey)) {
        directionX -= 1.0F;
    }
    if (isKeyPressed(window, controls.rightKey)) {
        directionX += 1.0F;
    }

    const float directionLength =
            std::sqrt(directionX * directionX + directionY * directionY);

    if (directionLength > 0.0F) {
        directionX /= directionLength;
        directionY /= directionLength;
        player.move(directionX, directionY, deltaTime);
    }

    player.keepInside(-FIELD_HALF_WIDTH, FIELD_HALF_WIDTH,
                      -FIELD_HALF_HEIGHT, FIELD_HALF_HEIGHT);
}

void handleTeamInput(GLFWwindow* window, Team& team,
                     TeamControlState& state, const TeamControls& controls,
                     const Ball& ball, float deltaTime) {
    updateControlledPlayerIndex(state, team, ball,
                                isKeyPressed(window, controls.switchKey));
    handlePlayerInput(window, getControlledPlayer(team, state),
                      controls, deltaTime);
}

void syncSwitchKeyState(GLFWwindow* window, TeamControlState& state,
                        const TeamControls& controls) {
    state.switchKeyWasPressed = isKeyPressed(window, controls.switchKey);
}

void resetControlledPlayer(TeamControlState& state) {
    state.controlledPlayerIndex = 0U;
}

bool resolvePlayerBallCollision(const Player& player, Ball& ball,
                                float playerVelocityX, float playerVelocityY,
                                bool allowImpulse) {
    const float deltaX = ball.getX() - player.getX();
    const float deltaY = ball.getY() - player.getY();
    const float minimumDistance = player.getRadius() + ball.getRadius();
    const float distanceSquared = deltaX * deltaX + deltaY * deltaY;

    if (distanceSquared > minimumDistance * minimumDistance) {
        return false;
    }

    float directionX;
    float directionY;
    const float distance = std::sqrt(distanceSquared);

    if (distance > 0.0001F) {
        directionX = deltaX / distance;
        directionY = deltaY / distance;
    } else {
        const float velocityLength = std::sqrt(
                ball.getVelocityX() * ball.getVelocityX()
                + ball.getVelocityY() * ball.getVelocityY());

        if (velocityLength > 0.0001F) {
            directionX = ball.getVelocityX() / velocityLength;
            directionY = ball.getVelocityY() / velocityLength;
        } else {
            directionX = 1.0F;
            directionY = 0.0F;
        }
    }

    ball.setX(player.getX() + directionX * minimumDistance);
    ball.setY(player.getY() + directionY * minimumDistance);
    const float relativeNormalVelocity =
            (ball.getVelocityX() - playerVelocityX) * directionX
            + (ball.getVelocityY() - playerVelocityY) * directionY;
    if (allowImpulse && relativeNormalVelocity < -0.0001F) {
        ball.setVelocityX(directionX * BALL_IMPACT_SPEED);
        ball.setVelocityY(directionY * BALL_IMPACT_SPEED);
        return true;
    }
    return false;
}

std::array<PlayerPosition, Team::MAX_PLAYERS> capturePlayerPositions(const Team& team) {
    std::array<PlayerPosition, Team::MAX_PLAYERS> positions{};
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        const Player& player = *team.getPlayers()[index];
        positions[index] = {player.getX(), player.getY()};
    }
    return positions;
}

void correctPlayerPosition(Player& player, float dx, float dy) {
    player.setX(player.getX() + dx);
    player.setY(player.getY() + dy);
    player.keepInside(-FIELD_HALF_WIDTH, FIELD_HALF_WIDTH,
                      -FIELD_HALF_HEIGHT, FIELD_HALF_HEIGHT);
}

void resolvePlayerPair(Player& a, bool aControlled, const PlayerPosition& previousA,
                       Player& b, bool bControlled, const PlayerPosition& previousB) {
    if (!aControlled && !bControlled) {
        return;
    }
    float dx = b.getX() - a.getX();
    float dy = b.getY() - a.getY();
    const float radius = a.getRadius() + b.getRadius();
    const float distanceSquared = dx * dx + dy * dy;
    if (distanceSquared >= radius * radius) {
        return;
    }

    const float distance = std::sqrt(distanceSquared);
    if (distance > 0.0001F) {
        dx /= distance;
        dy /= distance;
    } else {
        // Prefer the previous separation; otherwise choose an inward direction
        // for a lone controlled player. Two controlled players share the axis.
        dx = previousB.x - previousA.x;
        dy = previousB.y - previousA.y;
        const float previousDistance = std::hypot(dx, dy);
        if (previousDistance > 0.0001F) {
            dx /= previousDistance;
            dy /= previousDistance;
        } else {
            dx = aControlled ? (a.getX() >= 0.0F ? 1.0F : -1.0F)
                             : (b.getX() >= 0.0F ? -1.0F : 1.0F);
            dy = 0.0F;
        }
    }

    const float penetration = radius - distance;
    const float correctionA = aControlled ? penetration * (bControlled ? 0.5F : 1.0F) : 0.0F;
    const float correctionB = bControlled ? penetration * (aControlled ? 0.5F : 1.0F) : 0.0F;
    const float ax = a.getX();
    const float ay = a.getY();
    const float bx = b.getX();
    const float by = b.getY();
    if (aControlled) correctPlayerPosition(a, -dx * correctionA, -dy * correctionA);
    if (bControlled) correctPlayerPosition(b, dx * correctionB, dy * correctionB);

    // Transfer correction prevented by a wall to the other movable player.
    const float blockedA = std::max(0.0F, correctionA
            - ((ax - a.getX()) * dx + (ay - a.getY()) * dy));
    const float blockedB = std::max(0.0F, correctionB
            - ((b.getX() - bx) * dx + (b.getY() - by) * dy));
    if (bControlled) correctPlayerPosition(b, dx * blockedA, dy * blockedA);
    if (aControlled) correctPlayerPosition(a, -dx * blockedB, -dy * blockedB);
}

void resolvePlayerCollisions(
        Team& blueTeam, const TeamControlState& blueState,
        const std::array<PlayerPosition, Team::MAX_PLAYERS>& bluePrevious,
        Team& redTeam, const TeamControlState& redState,
        const std::array<PlayerPosition, Team::MAX_PLAYERS>& redPrevious) {
    constexpr std::size_t CAPACITY = 2U * Team::MAX_PLAYERS;
    std::array<Player*, CAPACITY> players{};
    std::array<PlayerPosition, CAPACITY> previous{};
    std::array<bool, CAPACITY> controlled{};
    std::size_t count = 0;
    for (std::size_t i = 0; i < blueTeam.getPlayerCount(); ++i, ++count) {
        players[count] = blueTeam.getPlayers()[i];
        previous[count] = bluePrevious[i];
        controlled[count] = i == blueState.controlledPlayerIndex;
    }
    for (std::size_t i = 0; i < redTeam.getPlayerCount(); ++i, ++count) {
        players[count] = redTeam.getPlayers()[i];
        previous[count] = redPrevious[i];
        controlled[count] = i == redState.controlledPlayerIndex;
    }

    constexpr int MAX_PASSES = 8;
    constexpr float OVERLAP_TOLERANCE = 0.0001F;
    for (int pass = 0; pass <= MAX_PASSES; ++pass) {
        std::array<PlayerPosition, CAPACITY> corrections{};
        bool overlapping = false;
        for (std::size_t i = 0; i < count; ++i) {
            for (std::size_t j = i + 1U; j < count; ++j) {
                if (!controlled[i] && !controlled[j]) continue;
                const float dx = players[j]->getX() - players[i]->getX();
                const float dy = players[j]->getY() - players[i]->getY();
                const float radius = players[i]->getRadius() + players[j]->getRadius();
                if (dx * dx + dy * dy >= (radius - OVERLAP_TOLERANCE)
                                                 * (radius - OVERLAP_TOLERANCE)) continue;
                overlapping = true;
                Player a = *players[i];
                Player b = *players[j];
                resolvePlayerPair(a, controlled[i], previous[i],
                                   b, controlled[j], previous[j]);
                corrections[i].x += a.getX() - players[i]->getX();
                corrections[i].y += a.getY() - players[i]->getY();
                corrections[j].x += b.getX() - players[j]->getX();
                corrections[j].y += b.getY() - players[j]->getY();
            }
        }
        if (!overlapping) return;
        if (pass == MAX_PASSES) break;
        // Apply simultaneous corrections from a common snapshot, not team order.
        for (std::size_t i = 0; i < count; ++i) {
            if (controlled[i]) correctPlayerPosition(*players[i], corrections[i].x, corrections[i].y);
        }
    }

    // A blocked cluster cannot push stationary teammates. Prefer the last valid
    // short-step positions to forcing a player through an obstacle or wall.
    bool previousValid = true;
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = i + 1U; j < count; ++j) {
            if (!controlled[i] && !controlled[j]) continue;
            const float dx = previous[j].x - previous[i].x;
            const float dy = previous[j].y - previous[i].y;
            const float radius = players[i]->getRadius() + players[j]->getRadius()
                                 - OVERLAP_TOLERANCE;
            if (dx * dx + dy * dy < radius * radius) previousValid = false;
        }
    }
    if (previousValid) {
        for (std::size_t i = 0; i < count; ++i) {
            if (controlled[i]) {
                players[i]->setX(previous[i].x);
                players[i]->setY(previous[i].y);
            }
        }
    }
}

void resolveTeamBallCollisions(
        const Team& team, const std::array<PlayerPosition, Team::MAX_PLAYERS>& previous,
        Ball& ball, float deltaTime, bool& impulseApplied) {
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        const Player& player = *team.getPlayers()[index];
        const float velocityX = deltaTime > 0.0F
                ? (player.getX() - previous[index].x) / deltaTime : 0.0F;
        const float velocityY = deltaTime > 0.0F
                ? (player.getY() - previous[index].y) / deltaTime : 0.0F;
        if (resolvePlayerBallCollision(player, ball, velocityX, velocityY,
                                       !impulseApplied)) {
            impulseApplied = true;
        }
    }
}

bool isBallPositionClear(const Team& blueTeam, const Team& redTeam,
                         const Ball& ball) {
    for (const Team* team : {&blueTeam, &redTeam}) {
        for (std::size_t index = 0; index < team->getPlayerCount(); ++index) {
            const Player& player = *team->getPlayers()[index];
            const float dx = ball.getX() - player.getX();
            const float dy = ball.getY() - player.getY();
            const float radius = player.getRadius() + ball.getRadius() - 0.0001F;
            if (dx * dx + dy * dy < radius * radius) {
                return false;
            }
        }
    }
    return true;
}

void releaseTrappedBall(const Team& blueTeam, const Team& redTeam, Ball& ball) {
    bool blockedMotion = false;
    for (const Team* team : {&blueTeam, &redTeam}) {
        for (std::size_t index = 0; index < team->getPlayerCount(); ++index) {
            const Player& player = *team->getPlayers()[index];
            const float dx = ball.getX() - player.getX();
            const float dy = ball.getY() - player.getY();
            const float radius = player.getRadius() + ball.getRadius() + 0.001F;
            if (dx * dx + dy * dy <= radius * radius
                && dx * ball.getVelocityX() + dy * ball.getVelocityY() < -0.0001F) {
                blockedMotion = true;
            }
        }
    }
    if (!blockedMotion && isBallPositionClear(blueTeam, redTeam, ball)) {
        return;
    }

    // Also escape opposing tangent contacts instead of bouncing in place.
    // Search fixed rays for the shortest free exit; ties keep the first ray.
    constexpr int DIRECTIONS = 16;
    const float originX = ball.getX();
    const float originY = ball.getY();
    float bestDistance = -1.0F;
    float bestX = originX;
    float bestY = originY;
    const std::size_t playerCount = blueTeam.getPlayerCount() + redTeam.getPlayerCount();
    for (int direction = 0; direction < DIRECTIONS; ++direction) {
        const double angle = 2.0 * PI * direction / DIRECTIONS;
        const float nx = static_cast<float>(std::cos(angle));
        const float ny = static_cast<float>(std::sin(angle));
        float distance = 0.0F;
        for (std::size_t pass = 0; pass < playerCount; ++pass) {
            for (const Team* team : {&blueTeam, &redTeam}) {
                for (std::size_t index = 0; index < team->getPlayerCount(); ++index) {
                    const Player& player = *team->getPlayers()[index];
                    const float dx = originX + nx * distance - player.getX();
                    const float dy = originY + ny * distance - player.getY();
                    const float radius = player.getRadius() + ball.getRadius() + 0.001F;
                    if (dx * dx + dy * dy < radius * radius) {
                        const float along = dx * nx + dy * ny;
                        distance += -along + std::sqrt(std::max(
                                0.0F, along * along + radius * radius - dx * dx - dy * dy));
                    }
                }
            }
        }
        Ball candidate = ball;
        candidate.setX(originX + nx * distance);
        candidate.setY(originY + ny * distance);
        candidate.update(0.0F, -FIELD_HALF_WIDTH, FIELD_HALF_WIDTH,
                         -FIELD_HALF_HEIGHT, FIELD_HALF_HEIGHT, GOAL_DEPTH, GOAL_HALF_HEIGHT);
        const bool insideBounds =
                std::abs(candidate.getX() - (originX + nx * distance)) < 0.0001F
                && std::abs(candidate.getY() - (originY + ny * distance)) < 0.0001F;
        if (insideBounds && isBallPositionClear(blueTeam, redTeam, candidate)
            && (bestDistance < 0.0F || distance < bestDistance)) {
            bestDistance = distance;
            bestX = candidate.getX();
            bestY = candidate.getY();
        }
    }
    if (bestDistance >= 0.0F) {
        const float dx = bestX - originX;
        const float dy = bestY - originY;
        const float length = std::sqrt(dx * dx + dy * dy);
        const float speed = std::hypot(ball.getVelocityX(), ball.getVelocityY());
        ball.setX(bestX);
        ball.setY(bestY);
        if (length > 0.0001F) {
            ball.setVelocityX(dx / length * speed);
            ball.setVelocityY(dy / length * speed);
        }
    }
}

bool updatePlaying(GLFWwindow* window,
                   Team& blueTeam, TeamControlState& blueControlState,
                   Team& redTeam, TeamControlState& redControlState,
                   Ball& ball,
                   float deltaTime) {
    // Bound travel per step, including the existing impact speed, to avoid
    // skipping a player during an ordinary frame or a temporary frame delay.
    const float maximumSpeed = std::max(BALL_IMPACT_SPEED,
            std::hypot(ball.getVelocityX(), ball.getVelocityY())) + PLAYER_SPEED;
    const int steps = std::max(1, static_cast<int>(std::ceil(
            deltaTime * maximumSpeed / (ball.getRadius() * 0.5F))));
    const float stepTime = deltaTime / static_cast<float>(steps);
    for (int step = 0; step < steps; ++step) {
        const auto bluePrevious = capturePlayerPositions(blueTeam);
        const auto redPrevious = capturePlayerPositions(redTeam);
        handleTeamInput(window, blueTeam, blueControlState,
                        BLUE_CONTROLS, ball, stepTime);
        handleTeamInput(window, redTeam, redControlState,
                        RED_CONTROLS, ball, stepTime);
        resolvePlayerCollisions(blueTeam, blueControlState, bluePrevious,
                                redTeam, redControlState, redPrevious);
        ball.update(stepTime, -FIELD_HALF_WIDTH, FIELD_HALF_WIDTH,
                    -FIELD_HALF_HEIGHT, FIELD_HALF_HEIGHT, GOAL_DEPTH, GOAL_HALF_HEIGHT);

        // Fixed Blue/Red, registration order. At most one impulse per step;
        // extra passes correct positions only, including wall constraints.
        bool impulseApplied = false;
        for (int pass = 0; pass < 4; ++pass) {
            resolveTeamBallCollisions(blueTeam, bluePrevious, ball, stepTime, impulseApplied);
            resolveTeamBallCollisions(redTeam, redPrevious, ball, stepTime, impulseApplied);
            impulseApplied = true;
            ball.update(0.0F, -FIELD_HALF_WIDTH, FIELD_HALF_WIDTH,
                        -FIELD_HALF_HEIGHT, FIELD_HALF_HEIGHT, GOAL_DEPTH, GOAL_HALF_HEIGHT);
            if (isBallPositionClear(blueTeam, redTeam, ball)) {
                break;
            }
        }
        releaseTrappedBall(blueTeam, redTeam, ball);
        if (updateScore(ball, blueTeam, redTeam)) {
            return true;
        }
    }
    return false;
}

void resetPlayers(Team& team,
                  const std::array<PlayerPosition, Team::MAX_PLAYERS>& positions) {
    const auto& players = team.getPlayers();
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        players[index]->setX(positions[index].x);
        players[index]->setY(positions[index].y);
    }
}

void resetPlay(Team& blueTeam, Team& redTeam, Ball& ball) {
    resetPlayers(blueTeam, BLUE_INITIAL_POSITIONS);
    resetPlayers(redTeam, RED_INITIAL_POSITIONS);
    ball.setX(BALL_INITIAL_X);
    ball.setY(BALL_INITIAL_Y);
    ball.setVelocityX(BALL_INITIAL_VELOCITY_X);
    ball.setVelocityY(BALL_INITIAL_VELOCITY_Y);
}

void drawPlayer(const Player& player) {
    constexpr int SEGMENTS = 48;

    if (player.getTeam() == TeamId::Blue) {
        glColor3f(0.1F, 0.3F, 0.95F);
    } else {
        glColor3f(0.9F, 0.15F, 0.15F);
    }

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(player.getX(), player.getY());

    for (int i = 0; i <= SEGMENTS; ++i) {
        const double angle = 2.0 * PI * i / SEGMENTS;
        glVertex2d(player.getX() + player.getRadius() * std::cos(angle),
                   player.getY() + player.getRadius() * std::sin(angle));
    }

    glEnd();
}

void drawTeam(const Team& team) {
    const auto& players = team.getPlayers();
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        drawPlayer(*players[index]);
    }
}

void drawControlledPlayerIndicator(const Player& player) {
    constexpr float RADIUS_OFFSET = 0.6F;

    glColor3f(1.0F, 1.0F, 1.0F);
    glLineWidth(2.0F);
    drawCircle(player.getX(), player.getY(), player.getRadius() + RADIUS_OFFSET);
}

void drawBall(const Ball& ball) {
    constexpr int SEGMENTS = 48;

    glColor3f(1.0F, 1.0F, 1.0F);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(ball.getX(), ball.getY());

    for (int i = 0; i <= SEGMENTS; ++i) {
        const double angle = 2.0 * PI * i / SEGMENTS;
        glVertex2d(ball.getX() + ball.getRadius() * std::cos(angle),
                   ball.getY() + ball.getRadius() * std::sin(angle));
    }

    glEnd();

    glColor3f(0.05F, 0.05F, 0.05F);
    glLineWidth(1.5F);
    drawCircle(ball.getX(), ball.getY(), ball.getRadius());
}

void drawField() {
    glColor3f(0.08F, 0.45F, 0.16F);
    glBegin(GL_QUADS);
    glVertex2f(-FIELD_HALF_WIDTH, -FIELD_HALF_HEIGHT);
    glVertex2f(FIELD_HALF_WIDTH, -FIELD_HALF_HEIGHT);
    glVertex2f(FIELD_HALF_WIDTH, FIELD_HALF_HEIGHT);
    glVertex2f(-FIELD_HALF_WIDTH, FIELD_HALF_HEIGHT);
    glEnd();

    glColor3f(1.0F, 1.0F, 1.0F);
    glLineWidth(2.0F);

    drawRectangle(-FIELD_HALF_WIDTH, -FIELD_HALF_HEIGHT,
                  FIELD_HALF_WIDTH, FIELD_HALF_HEIGHT);

    glBegin(GL_LINES);
    glVertex2f(0.0F, -FIELD_HALF_HEIGHT);
    glVertex2f(0.0F, FIELD_HALF_HEIGHT);
    glEnd();

    drawCircle(0.0F, 0.0F, 9.15F);

    drawRectangle(-FIELD_HALF_WIDTH, -20.16F,
                  -FIELD_HALF_WIDTH + 16.5F, 20.16F);
    drawRectangle(FIELD_HALF_WIDTH - 16.5F, -20.16F,
                  FIELD_HALF_WIDTH, 20.16F);

    constexpr float PENALTY_ARC_RADIUS = 9.15F;
    const double penaltyArcHalfAngle =
            std::acos(5.5 / PENALTY_ARC_RADIUS);

    drawArc(-FIELD_HALF_WIDTH + 11.0F, 0.0F, PENALTY_ARC_RADIUS,
            -penaltyArcHalfAngle, penaltyArcHalfAngle);
    drawArc(FIELD_HALF_WIDTH - 11.0F, 0.0F, PENALTY_ARC_RADIUS,
            PI - penaltyArcHalfAngle, PI + penaltyArcHalfAngle);

    drawRectangle(-FIELD_HALF_WIDTH, -9.16F,
                  -FIELD_HALF_WIDTH + 5.5F, 9.16F);
    drawRectangle(FIELD_HALF_WIDTH - 5.5F, -9.16F,
                  FIELD_HALF_WIDTH, 9.16F);

    drawRectangle(-FIELD_HALF_WIDTH - GOAL_DEPTH, -GOAL_HALF_HEIGHT,
                  -FIELD_HALF_WIDTH, GOAL_HALF_HEIGHT);
    drawRectangle(FIELD_HALF_WIDTH, -GOAL_HALF_HEIGHT,
                  FIELD_HALF_WIDTH + GOAL_DEPTH, GOAL_HALF_HEIGHT);
}
} // namespace

int main() {
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Falha ao inicializar o GLFW.\n";
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "KickOff", nullptr, nullptr);

    if (window == nullptr) {
        std::cerr << "Falha ao criar a janela.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, configureProjection);

    int framebufferWidth;
    int framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    configureProjection(window, framebufferWidth, framebufferHeight);

    std::array<Player, Team::MAX_PLAYERS> bluePlayers{{
        Player(BLUE_INITIAL_POSITIONS[0].x, BLUE_INITIAL_POSITIONS[0].y,
               PLAYER_SPEED, PLAYER_RADIUS, TeamId::Blue),
        Player(BLUE_INITIAL_POSITIONS[1].x, BLUE_INITIAL_POSITIONS[1].y,
               PLAYER_SPEED, PLAYER_RADIUS, TeamId::Blue),
        Player(BLUE_INITIAL_POSITIONS[2].x, BLUE_INITIAL_POSITIONS[2].y,
               PLAYER_SPEED, PLAYER_RADIUS, TeamId::Blue)
    }};
    std::array<Player, Team::MAX_PLAYERS> redPlayers{{
        Player(RED_INITIAL_POSITIONS[0].x, RED_INITIAL_POSITIONS[0].y,
               PLAYER_SPEED, PLAYER_RADIUS, TeamId::Red),
        Player(RED_INITIAL_POSITIONS[1].x, RED_INITIAL_POSITIONS[1].y,
               PLAYER_SPEED, PLAYER_RADIUS, TeamId::Red),
        Player(RED_INITIAL_POSITIONS[2].x, RED_INITIAL_POSITIONS[2].y,
               PLAYER_SPEED, PLAYER_RADIUS, TeamId::Red)
    }};

    Ball ball(BALL_INITIAL_X, BALL_INITIAL_Y,
              BALL_INITIAL_VELOCITY_X, BALL_INITIAL_VELOCITY_Y,
              BALL_RADIUS);
    Team blueTeam(TeamId::Blue, "Blue");
    Team redTeam(TeamId::Red, "Red");
    for (Player& player : bluePlayers) {
        blueTeam.addPlayer(player);
    }
    for (Player& player : redPlayers) {
        redTeam.addPlayer(player);
    }
    TeamControlState blueControlState{0U, false};
    TeamControlState redControlState{0U, false};
    MatchState matchState = MatchState::Playing;
    float goalStateElapsed = 0.0F;

    double previousTime = glfwGetTime();

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        if (matchState == MatchState::Playing) {
            if (updatePlaying(window,
                              blueTeam, blueControlState,
                              redTeam, redControlState,
                              ball, deltaTime)) {
                matchState = MatchState::Goal;
                goalStateElapsed = 0.0F;
                resetControlledPlayer(blueControlState);
                resetControlledPlayer(redControlState);
            }
        } else {
            syncSwitchKeyState(window, blueControlState, BLUE_CONTROLS);
            syncSwitchKeyState(window, redControlState, RED_CONTROLS);
            goalStateElapsed += deltaTime;
            if (goalStateElapsed >= GOAL_STATE_DURATION) {
                resetPlay(blueTeam, redTeam, ball);
                matchState = MatchState::Playing;
            }
        }
        glClearColor(0.03F, 0.12F, 0.05F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();
        drawField();
        drawTeam(blueTeam);
        drawTeam(redTeam);
        drawBall(ball);
        drawControlledPlayerIndicator(getControlledPlayer(blueTeam, blueControlState));
        drawControlledPlayerIndicator(getControlledPlayer(redTeam, redControlState));
        drawScore(blueTeam, redTeam);
        if (matchState == MatchState::Goal) {
            drawGoalMessage();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
