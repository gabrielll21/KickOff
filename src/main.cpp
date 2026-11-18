#include "entities/Ball.h"
#include "entities/Player.h"
#include "entities/Team.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {
constexpr double WORLD_WIDTH = 120.0;
constexpr double WORLD_HEIGHT = 80.0;
constexpr float FIELD_HALF_WIDTH = 52.5F;
constexpr float PLAYER_SPEED = 12.0F;
constexpr float AI_MOVEMENT_SPEED = 0.6F * PLAYER_SPEED;
constexpr float AI_BALL_INFLUENCE = 0.15F;
constexpr float AI_MAX_OFFSET_X = 12.0F;
constexpr float AI_MAX_OFFSET_Y = 8.0F;
constexpr float AI_STOP_DISTANCE = 0.5F;
constexpr float POSSESSION_SWITCH_MARGIN = 3.0F;
constexpr float ATTACK_FORWARD_OFFSET = 6.0F;
constexpr float DEFENSE_BACK_OFFSET = 5.0F;
constexpr float MARKING_DISTANCE = 5.0F;
constexpr float DEFENSIVE_BASE_WEIGHT = 0.4F;
constexpr float MARKING_TARGET_WEIGHT = 0.6F;
constexpr float MARKING_SWITCH_MARGIN = 2.0F;
constexpr std::size_t NO_MARKING_TARGET = Team::MAX_PLAYERS;
constexpr float SUPPORT_LATERAL_OFFSET = 4.0F;
constexpr float SUPPORT_FORWARD_OFFSET = 3.0F;
constexpr float PASS_LANE_CLEARANCE = 4.0F;
constexpr float SUPPORT_MIN_SPACING = 8.0F;
constexpr float SUPPORT_BASE_WEIGHT = 1.0F;
constexpr float SUPPORT_LANE_WEIGHT = 3.0F;
constexpr float SUPPORT_SPACING_WEIGHT = 2.0F;
constexpr float SUPPORT_SELECTED_WEIGHT = 2.0F;
constexpr float SUPPORT_SWITCH_MARGIN = 2.0F;
constexpr float SUPPORT_POSITION_EPSILON = 0.0001F;
constexpr std::size_t SUPPORT_CANDIDATE_COUNT = 9U;
constexpr std::size_t NO_SUPPORT_CANDIDATE = SUPPORT_CANDIDATE_COUNT;
constexpr float PLAYER_RADIUS = 2.0F;
constexpr float BALL_INITIAL_X = 0.0F;
constexpr float BALL_INITIAL_Y = 0.0F;
constexpr float BALL_INITIAL_VELOCITY_X = 14.0F;
constexpr float BALL_INITIAL_VELOCITY_Y = 9.0F;
constexpr float BALL_RADIUS = 1.0F;
constexpr float BALL_IMPACT_SPEED = 20.0F;
constexpr float BALL_KICK_SPEED = 35.0F;
constexpr float BALL_PASS_SPEED = 25.0F;
constexpr float FIELD_HALF_HEIGHT = 34.0F;
constexpr float GOAL_DEPTH = 2.44F;
constexpr float GOAL_OPENING_SCALE = 1.4F;
constexpr float GOAL_HALF_HEIGHT = 3.66F * GOAL_OPENING_SCALE;
constexpr float GOAL_STATE_DURATION = 2.0F;
constexpr double MATCH_DURATION_SECONDS = 180.0;
constexpr double TIME_ROUNDING_TOLERANCE = 1e-10;
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
    int kickKey;
    int passKey;
};

struct DefensiveMarking {
    std::size_t selectedIndex = NO_MARKING_TARGET;
    std::array<std::size_t, Team::MAX_PLAYERS> opponents{{
        NO_MARKING_TARGET, NO_MARKING_TARGET, NO_MARKING_TARGET
    }};
};

struct OffensiveSupport {
    std::size_t selectedIndex = Team::MAX_PLAYERS;
    std::array<std::size_t, Team::MAX_PLAYERS> candidates{{
        NO_SUPPORT_CANDIDATE, NO_SUPPORT_CANDIDATE, NO_SUPPORT_CANDIDATE
    }};
};

struct SupportCandidates {
    std::array<PlayerPosition, SUPPORT_CANDIDATE_COUNT> positions{};
    std::array<bool, SUPPORT_CANDIDATE_COUNT> available{};
};

struct TeamControlState {
    std::size_t controlledPlayerIndex;
    bool switchKeyWasPressed;
    bool kickKeyWasPressed = false;
    bool passKeyWasPressed = false;
    const Player* initialContactPlayer = nullptr;
    DefensiveMarking marking{};
    OffensiveSupport support{};
};

struct BallAction {
    Player* origin = nullptr;
    Player* receiver = nullptr;
    std::size_t receiverIndex = 0U;
    float distanceSquared = 0.0F;
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
    GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_TAB, GLFW_KEY_SPACE,
    GLFW_KEY_Q
};
constexpr TeamControls RED_CONTROLS{
    GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_ENTER,
    GLFW_KEY_RIGHT_SHIFT, GLFW_KEY_RIGHT_CONTROL
};

enum class GoalSide { None, Left, Right };
enum class MatchState { Menu, Playing, Goal, Finished };
struct MatchProgress {
    MatchState state = MatchState::Menu;
    double remainingTime = MATCH_DURATION_SECONDS;
    double goalElapsed = 0.0;
    bool restartWasPressed = false;
    bool enterWasPressed = false;
    bool menuWasPressed = false;
};
enum class EstimatedPossession { Neutral, Blue, Red };
enum class TacticalState { Neutral, Attacking, Defending };

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

bool consumeKeyPress(bool pressed, bool& wasPressed) {
    const bool justPressed = pressed && !wasPressed;
    wasPressed = pressed;
    return justPressed;
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
    if (consumeKeyPress(switchKeyPressed, state.switchKeyWasPressed)
        && team.getPlayerCount() > 0U) {
        const std::size_t next = findClosestPlayerIndex(team, ball, state.controlledPlayerIndex);
        if (next != state.controlledPlayerIndex) {
            state.marking = {};
            state.support = {};
        }
        state.controlledPlayerIndex = next;
    }
}

Player& getControlledPlayer(Team& team, const TeamControlState& state) {
    return *team.getPlayers()[state.controlledPlayerIndex];
}

PlayerPosition readMovementDirection(GLFWwindow* window, const TeamControls& controls) {
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
    }
    return {directionX, directionY};
}

void handlePlayerInput(GLFWwindow* window, Player& player,
                       const TeamControls& controls, float deltaTime) {
    const PlayerPosition direction = readMovementDirection(window, controls);
    player.move(direction.x, direction.y, deltaTime);
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

void syncTeamInputState(GLFWwindow* window, TeamControlState& state,
                        const TeamControls& controls) {
    state.switchKeyWasPressed = isKeyPressed(window, controls.switchKey);
    state.kickKeyWasPressed = isKeyPressed(window, controls.kickKey);
    state.passKeyWasPressed = isKeyPressed(window, controls.passKey);
    state.initialContactPlayer = nullptr;
}

void resetControlledPlayer(TeamControlState& state) {
    state.controlledPlayerIndex = 0U;
    state.initialContactPlayer = nullptr;
    state.marking = {};
    state.support = {};
}

float kickDistanceSquared(const Player& player, const Ball& ball) {
    const float dx = player.getX() - ball.getX();
    const float dy = player.getY() - ball.getY();
    return dx * dx + dy * dy;
}

bool canKick(const Player& player, const Ball& ball) {
    const float range = player.getRadius() + ball.getRadius() + 0.5F;
    return kickDistanceSquared(player, ball) <= range * range;
}

std::size_t findPassReceiverIndex(const Team& team, std::size_t originIndex) {
    const Player& origin = *team.getPlayers()[originIndex];
    std::size_t nearest = team.getPlayerCount();
    float nearestDistance = 0.0F;
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        if (index == originIndex) continue;
        const Player& candidate = *team.getPlayers()[index];
        const float dx = candidate.getX() - origin.getX();
        const float dy = candidate.getY() - origin.getY();
        const float distance = dx * dx + dy * dy;
        // Ascending registration order keeps the lowest index on a tie.
        if (nearest == team.getPlayerCount() || distance < nearestDistance) {
            nearest = index;
            nearestDistance = distance;
        }
    }
    return nearest;
}

bool hasPassDirection(const Player& receiver, const Ball& ball) {
    return kickDistanceSquared(receiver, ball) > 0.0F;
}

BallAction prepareBallAction(Team& team, const TeamControlState& state,
                             const Ball& ball, bool kickRequested, bool passRequested) {
    if (team.getPlayerCount() == 0U || (!kickRequested && !passRequested)) return {};
    Player& origin = getControlledPlayer(team, state);
    if (!canKick(origin, ball)) return {};
    BallAction action;
    action.origin = &origin;
    action.distanceSquared = kickDistanceSquared(origin, ball);
    if (kickRequested) return action;
    const std::size_t receiverIndex = findPassReceiverIndex(team, state.controlledPlayerIndex);
    if (receiverIndex == team.getPlayerCount()) return {};
    action.receiver = team.getPlayers()[receiverIndex];
    action.receiverIndex = receiverIndex;
    if (!hasPassDirection(*action.receiver, ball)) return {};
    return action;
}

BallAction chooseBallAction(const BallAction& blue, const BallAction& red, const Ball& ball) {
    // Contact correction may have moved the ball onto a receiver. An invalid
    // action must not block the opponent's valid action or change selection.
    const bool blueValid = blue.origin != nullptr
            && (blue.receiver == nullptr || hasPassDirection(*blue.receiver, ball));
    const bool redValid = red.origin != nullptr
            && (red.receiver == nullptr || hasPassDirection(*red.receiver, ball));
    if (blueValid && redValid) {
        if (blue.distanceSquared == red.distanceSquared) return {};
        return blue.distanceSquared < red.distanceSquared ? blue : red;
    }
    return blueValid ? blue : (redValid ? red : BallAction{});
}

bool applyPass(const BallAction& action, TeamControlState& state, Ball& ball) {
    const float dx = action.receiver->getX() - ball.getX();
    const float dy = action.receiver->getY() - ball.getY();
    const float length = std::hypot(dx, dy);
    if (length == 0.0F) return false;
    ball.setVelocityX(dx / length * BALL_PASS_SPEED);
    ball.setVelocityY(dy / length * BALL_PASS_SPEED);
    state.controlledPlayerIndex = action.receiverIndex;
    state.marking = {};
    state.support = {};
    return true;
}

void applyKick(GLFWwindow* window, const Player& player,
               const TeamControls& controls, Ball& ball) {
    PlayerPosition direction = readMovementDirection(window, controls);
    if (direction.x == 0.0F && direction.y == 0.0F) {
        direction.x = player.getTeam() == TeamId::Blue ? 1.0F : -1.0F;
    }
    ball.setVelocityX(direction.x * BALL_KICK_SPEED);
    ball.setVelocityY(direction.y * BALL_KICK_SPEED);
}

const Player* protectedInitialContact(TeamControlState& state, const Ball& ball) {
    // Protect only the originating player's initial reach interaction, not
    // other players or later return contacts. Separation remains enabled.
    if (state.initialContactPlayer != nullptr) {
        const float range = state.initialContactPlayer->getRadius() + ball.getRadius()
                            + 0.5F + 0.0001F;
        if (kickDistanceSquared(*state.initialContactPlayer, ball) > range * range) {
            state.initialContactPlayer = nullptr;
        }
    }
    return state.initialContactPlayer;
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

float nearestTeamBallDistance(const Team& team, const Ball& ball) {
    if (team.getPlayerCount() == 0U) return std::numeric_limits<float>::infinity();
    const std::size_t index = findClosestPlayerIndex(team, ball, 0U);
    return std::sqrt(kickDistanceSquared(*team.getPlayers()[index], ball));
}

EstimatedPossession estimatePossession(const Team& blueTeam, const Team& redTeam,
                                       const Ball& ball, EstimatedPossession previous) {
    const float blueDistance = nearestTeamBallDistance(blueTeam, ball);
    const float redDistance = nearestTeamBallDistance(redTeam, ball);
    // Ties and the ambiguity band retain the previous estimate, including
    // Neutral. The margin is linear, not a difference of squared distances.
    if (blueDistance == redDistance) return previous;
    if (blueDistance + POSSESSION_SWITCH_MARGIN <= redDistance) return EstimatedPossession::Blue;
    if (redDistance + POSSESSION_SWITCH_MARGIN <= blueDistance) return EstimatedPossession::Red;
    return previous;
}

TacticalState teamTacticalState(TeamId id, EstimatedPossession possession) {
    if (possession == EstimatedPossession::Neutral) return TacticalState::Neutral;
    const bool attacking = (id == TeamId::Blue && possession == EstimatedPossession::Blue)
                           || (id == TeamId::Red && possession == EstimatedPossession::Red);
    return attacking ? TacticalState::Attacking : TacticalState::Defending;
}

PlayerPosition limitSupportTarget(const Player& player, const PlayerPosition& home,
                                   const PlayerPosition& target) {
    const float x = std::max(home.x - AI_MAX_OFFSET_X, std::min(target.x, home.x + AI_MAX_OFFSET_X));
    const float y = std::max(home.y - AI_MAX_OFFSET_Y, std::min(target.y, home.y + AI_MAX_OFFSET_Y));
    return {
        std::max(-FIELD_HALF_WIDTH + player.getRadius(), std::min(x, FIELD_HALF_WIDTH - player.getRadius())),
        std::max(-FIELD_HALF_HEIGHT + player.getRadius(), std::min(y, FIELD_HALF_HEIGHT - player.getRadius()))
    };
}

PlayerPosition automaticSupportTarget(const Player& player,
                                      const PlayerPosition& home, const Ball& ball,
                                      TacticalState tacticalState = TacticalState::Neutral) {
    const float forward = player.getTeam() == TeamId::Blue ? 1.0F : -1.0F;
    const float tacticalOffset = tacticalState == TacticalState::Attacking
            ? forward * ATTACK_FORWARD_OFFSET
            : (tacticalState == TacticalState::Defending ? -forward * DEFENSE_BACK_OFFSET : 0.0F);
    return limitSupportTarget(player, home, {
        home.x + (ball.getX() * AI_BALL_INFLUENCE + tacticalOffset),
        home.y + ball.getY() * AI_BALL_INFLUENCE
    });
}

float markingAssignmentCost(const PlayerPosition& home, const Player& opponent,
                             TeamId defendingTeam, const Ball& ball) {
    const float goalX = defendingTeam == TeamId::Blue ? -FIELD_HALF_WIDTH : FIELD_HALF_WIDTH;
    // All terms are linear distances. Region dominates; goal/ball proximity
    // provide modest threat weights shared by both defenders.
    return std::hypot(opponent.getX() - home.x, opponent.getY() - home.y)
           + 0.2F * std::hypot(opponent.getX() - goalX, opponent.getY())
           + 0.1F * std::hypot(opponent.getX() - ball.getX(), opponent.getY() - ball.getY());
}

bool validDefensiveMarking(const Team& team, const Team& opponents,
                            const TeamControlState& state) {
    if (team.getPlayerCount() != Team::MAX_PLAYERS
        || state.controlledPlayerIndex >= team.getPlayerCount()
        || state.marking.selectedIndex != state.controlledPlayerIndex) return false;
    std::array<bool, Team::MAX_PLAYERS> used{};
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        const std::size_t opponent = state.marking.opponents[index];
        if (index == state.controlledPlayerIndex) {
            if (opponent != NO_MARKING_TARGET) return false;
        } else {
            if (opponent >= opponents.getPlayerCount() || used[opponent]) return false;
            used[opponent] = true;
        }
    }
    return true;
}

void updateDefensiveMarking(const Team& team, const Team& opponents, TeamControlState& state,
                            const std::array<PlayerPosition, Team::MAX_PLAYERS>& homes,
                            const Ball& ball, TacticalState tacticalState) {
    if (tacticalState != TacticalState::Defending
        || team.getPlayerCount() != Team::MAX_PLAYERS
        || state.controlledPlayerIndex >= team.getPlayerCount()
        || opponents.getPlayerCount() < 2U) {
        state.marking = {};
        return;
    }
    std::array<std::size_t, 2> defenders{};
    std::size_t count = 0;
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        if (index != state.controlledPlayerIndex) defenders[count++] = index;
    }
    std::array<std::array<float, Team::MAX_PLAYERS>, 2> costs{};
    for (std::size_t defender = 0; defender < defenders.size(); ++defender) {
        for (std::size_t opponent = 0; opponent < opponents.getPlayerCount(); ++opponent) {
            costs[defender][opponent] = markingAssignmentCost(
                    homes[defenders[defender]], *opponents.getPlayers()[opponent], team.getId(), ball);
        }
    }
    float bestCost = std::numeric_limits<float>::infinity();
    std::size_t first = NO_MARKING_TARGET;
    std::size_t second = NO_MARKING_TARGET;
    // Compare all six distinct pairs as a set. Ascending indices break ties.
    for (std::size_t a = 0; a < opponents.getPlayerCount(); ++a) {
        for (std::size_t b = 0; b < opponents.getPlayerCount(); ++b) {
            if (a == b) continue;
            const float cost = costs[0][a] + costs[1][b];
            if (cost < bestCost) { bestCost = cost; first = a; second = b; }
        }
    }
    if (validDefensiveMarking(team, opponents, state)) {
        const float oldCost = costs[0][state.marking.opponents[defenders[0]]]
                              + costs[1][state.marking.opponents[defenders[1]]];
        if (oldCost <= bestCost + MARKING_SWITCH_MARGIN) return;
    }
    state.marking = {};
    state.marking.selectedIndex = state.controlledPlayerIndex;
    state.marking.opponents[defenders[0]] = first;
    state.marking.opponents[defenders[1]] = second;
}

PlayerPosition goalSideMarkingPoint(TeamId defendingTeam, const Player& opponent) {
    const float goalX = defendingTeam == TeamId::Blue ? -FIELD_HALF_WIDTH : FIELD_HALF_WIDTH;
    const float dx = goalX - opponent.getX();
    const float dy = -opponent.getY();
    const float distance = std::hypot(dx, dy);
    if (distance <= 0.0001F) return {opponent.getX(), opponent.getY()};
    const float travel = std::min(MARKING_DISTANCE, distance);
    return {opponent.getX() + dx / distance * travel, opponent.getY() + dy / distance * travel};
}

PlayerPosition defensiveZoneTarget(const Player& player, const PlayerPosition& home,
                                    const Ball& ball, const Player& opponent) {
    const PlayerPosition base = automaticSupportTarget(player, home, ball, TacticalState::Defending);
    const PlayerPosition marking = goalSideMarkingPoint(player.getTeam(), opponent);
    return limitSupportTarget(player, home, {
        base.x * DEFENSIVE_BASE_WEIGHT + marking.x * MARKING_TARGET_WEIGHT,
        base.y * DEFENSIVE_BASE_WEIGHT + marking.y * MARKING_TARGET_WEIGHT
    });
}

float pointSegmentDistance(const PlayerPosition& point, const PlayerPosition& a,
                            const PlayerPosition& b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float lengthSquared = dx * dx + dy * dy;
    float projection = 0.0F;
    if (lengthSquared > SUPPORT_POSITION_EPSILON * SUPPORT_POSITION_EPSILON) {
        projection = std::max(0.0F, std::min(1.0F,
                ((point.x - a.x) * dx + (point.y - a.y) * dy) / lengthSquared));
    }
    return std::hypot(point.x - (a.x + projection * dx), point.y - (a.y + projection * dy));
}

SupportCandidates offensiveSupportCandidates(const Player& player, const PlayerPosition& home,
                                              const PlayerPosition& base) {
    const float forward = player.getTeam() == TeamId::Blue
            ? SUPPORT_FORWARD_OFFSET : -SUPPORT_FORWARD_OFFSET;
    const std::array<PlayerPosition, SUPPORT_CANDIDATE_COUNT> offsets{{
        {0.0F, 0.0F}, {0.0F, SUPPORT_LATERAL_OFFSET}, {0.0F, -SUPPORT_LATERAL_OFFSET},
        {forward, 0.0F}, {-forward, 0.0F},
        {forward, SUPPORT_LATERAL_OFFSET}, {forward, -SUPPORT_LATERAL_OFFSET},
        {-forward, SUPPORT_LATERAL_OFFSET}, {-forward, -SUPPORT_LATERAL_OFFSET}
    }};
    SupportCandidates candidates;
    for (std::size_t index = 0; index < offsets.size(); ++index) {
        const PlayerPosition target = limitSupportTarget(player, home,
                {base.x + offsets[index].x, base.y + offsets[index].y});
        bool duplicate = false;
        for (std::size_t previous = 0; previous < index; ++previous) {
            if (candidates.available[previous]
                && std::hypot(target.x - candidates.positions[previous].x,
                              target.y - candidates.positions[previous].y) <= SUPPORT_POSITION_EPSILON) {
                duplicate = true;
                break;
            }
        }
        // Keep fixed offset identities: removing a duplicate must not renumber
        // the cached candidates when the ball or a regional clamp moves.
        candidates.positions[index] = target;
        candidates.available[index] = !duplicate;
    }
    return candidates;
}

float passLanePenalty(const PlayerPosition& origin, const PlayerPosition& target,
                       const Team& opponents) {
    float penalty = 0.0F;
    for (std::size_t index = 0; index < opponents.getPlayerCount(); ++index) {
        const Player& opponent = *opponents.getPlayers()[index];
        penalty += std::max(0.0F, PASS_LANE_CLEARANCE
                - pointSegmentDistance({opponent.getX(), opponent.getY()}, origin, target));
    }
    return penalty;
}

float supportSpacingPenalty(const PlayerPosition& a, const PlayerPosition& b) {
    return std::max(0.0F, SUPPORT_MIN_SPACING - std::hypot(a.x - b.x, a.y - b.y));
}

float supportCandidateCost(const PlayerPosition& origin, const PlayerPosition& target,
                            const PlayerPosition& base, const Team& opponents) {
    // Linear costs: formation remains the reference; blocked lanes cost more
    // than a small adjustment, and crowding is penalized without physical forces.
    return SUPPORT_BASE_WEIGHT * std::hypot(target.x - base.x, target.y - base.y)
           + SUPPORT_LANE_WEIGHT * passLanePenalty(origin, target, opponents)
           + SUPPORT_SELECTED_WEIGHT * supportSpacingPenalty(origin, target);
}

bool validOffensiveSupport(const Team& team, const TeamControlState& state,
                           const std::array<SupportCandidates, Team::MAX_PLAYERS>& candidates) {
    if (team.getPlayerCount() != Team::MAX_PLAYERS
        || state.controlledPlayerIndex >= team.getPlayerCount()
        || state.support.selectedIndex != state.controlledPlayerIndex) return false;
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        const std::size_t candidate = state.support.candidates[index];
        if (index == state.controlledPlayerIndex) {
            if (candidate != NO_SUPPORT_CANDIDATE) return false;
        } else if (candidate >= SUPPORT_CANDIDATE_COUNT || !candidates[index].available[candidate]) {
            return false;
        }
    }
    return true;
}

void chooseOffensiveSupportTargets(const Team& team, const Team& opponents,
                                   TeamControlState& state,
                                   const std::array<PlayerPosition, Team::MAX_PLAYERS>& homes,
                                   std::array<PlayerPosition, Team::MAX_PLAYERS>& targets) {
    std::array<std::size_t, 2> companions{};
    std::array<SupportCandidates, Team::MAX_PLAYERS> candidates{};
    std::array<std::array<float, SUPPORT_CANDIDATE_COUNT>, Team::MAX_PLAYERS> costs{};
    const Player& selected = *team.getPlayers()[state.controlledPlayerIndex];
    const PlayerPosition origin{selected.getX(), selected.getY()};
    std::size_t count = 0;
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        if (index == state.controlledPlayerIndex) continue;
        companions[count++] = index;
        candidates[index] = offensiveSupportCandidates(*team.getPlayers()[index], homes[index], targets[index]);
        for (std::size_t candidate = 0; candidate < SUPPORT_CANDIDATE_COUNT; ++candidate) {
            if (candidates[index].available[candidate]) {
                costs[index][candidate] = supportCandidateCost(origin,
                        candidates[index].positions[candidate], targets[index], opponents);
            }
        }
    }
    const std::size_t first = companions[0], second = companions[1];
    const auto pairCost = [&](std::size_t a, std::size_t b) {
        return costs[first][a] + costs[second][b]
               + SUPPORT_SPACING_WEIGHT * supportSpacingPenalty(
                       candidates[first].positions[a], candidates[second].positions[b]);
    };
    float bestCost = std::numeric_limits<float>::infinity();
    std::size_t bestA = 0U, bestB = 0U;
    // At most 81 pairs. Stable offset/registration order breaks exact ties.
    for (std::size_t a = 0; a < SUPPORT_CANDIDATE_COUNT; ++a) {
        if (!candidates[first].available[a]) continue;
        for (std::size_t b = 0; b < SUPPORT_CANDIDATE_COUNT; ++b) {
            if (!candidates[second].available[b]) continue;
            const float cost = pairCost(a, b);
            if (cost < bestCost) { bestCost = cost; bestA = a; bestB = b; }
        }
    }
    if (validOffensiveSupport(team, state, candidates)) {
        const std::size_t oldA = state.support.candidates[first];
        const std::size_t oldB = state.support.candidates[second];
        // Re-evaluate the previous pair with today's geometry, not a saved cost.
        if (pairCost(oldA, oldB) <= bestCost + SUPPORT_SWITCH_MARGIN) {
            bestA = oldA;
            bestB = oldB;
        }
    }
    state.support = {};
    state.support.selectedIndex = state.controlledPlayerIndex;
    state.support.candidates[first] = bestA;
    state.support.candidates[second] = bestB;
    targets[first] = candidates[first].positions[bestA];
    targets[second] = candidates[second].positions[bestB];
}

std::array<PlayerPosition, Team::MAX_PLAYERS> companionSupportTargets(
                        const Team& team, TeamControlState& state,
                        const std::array<PlayerPosition, Team::MAX_PLAYERS>& homes,
                        const Ball& ball,
                        TacticalState tacticalState = TacticalState::Neutral,
                        const Team* opponents = nullptr) {
    std::array<PlayerPosition, Team::MAX_PLAYERS> targets{};
    const bool marking = tacticalState == TacticalState::Defending && opponents != nullptr
                         && validDefensiveMarking(team, *opponents, state);
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        if (index == state.controlledPlayerIndex) continue;
        const Player& player = *team.getPlayers()[index];
        targets[index] = automaticSupportTarget(player, homes[index], ball, tacticalState);
        if (marking) {
            targets[index] = defensiveZoneTarget(player, homes[index], ball,
                                                 *opponents->getPlayers()[state.marking.opponents[index]]);
        }
    }
    if (tacticalState == TacticalState::Attacking && opponents != nullptr
        && team.getPlayerCount() == Team::MAX_PLAYERS
        && state.controlledPlayerIndex < team.getPlayerCount()) {
        chooseOffensiveSupportTargets(team, *opponents, state, homes, targets);
    } else {
        state.support = {};
    }
    return targets;
}

void moveTeamCompanions(Team& team, const TeamControlState& state,
                        const std::array<PlayerPosition, Team::MAX_PLAYERS>& targets, float stepTime) {
    if (stepTime <= 0.0F) return;
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        if (index == state.controlledPlayerIndex) continue;
        Player& player = *team.getPlayers()[index];
        const PlayerPosition& target = targets[index];
        const float dx = target.x - player.getX();
        const float dy = target.y - player.getY();
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared <= AI_STOP_DISTANCE * AI_STOP_DISTANCE) continue;
        const float distance = std::sqrt(distanceSquared);
        // Stop at the edge of the dead zone, including on a long step.
        const float travel = std::min(AI_MOVEMENT_SPEED * stepTime,
                                      distance - AI_STOP_DISTANCE);
        correctPlayerPosition(player, dx / distance * travel, dy / distance * travel);
    }
}

void resolvePlayerPair(Player& a, bool aMovable, const PlayerPosition& previousA,
                       Player& b, bool bMovable, const PlayerPosition& previousB) {
    if (!aMovable && !bMovable) {
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
        // for a lone moving player. Two moving players share the axis.
        dx = previousB.x - previousA.x;
        dy = previousB.y - previousA.y;
        const float previousDistance = std::hypot(dx, dy);
        if (previousDistance > 0.0001F) {
            dx /= previousDistance;
            dy /= previousDistance;
        } else {
            dx = aMovable ? (a.getX() >= 0.0F ? 1.0F : -1.0F)
                             : (b.getX() >= 0.0F ? -1.0F : 1.0F);
            dy = 0.0F;
        }
    }

    const float penetration = radius - distance;
    const float correctionA = aMovable ? penetration * (bMovable ? 0.5F : 1.0F) : 0.0F;
    const float correctionB = bMovable ? penetration * (aMovable ? 0.5F : 1.0F) : 0.0F;
    const float ax = a.getX();
    const float ay = a.getY();
    const float bx = b.getX();
    const float by = b.getY();
    if (aMovable) correctPlayerPosition(a, -dx * correctionA, -dy * correctionA);
    if (bMovable) correctPlayerPosition(b, dx * correctionB, dy * correctionB);

    // Transfer correction prevented by a wall to the other movable player.
    const float blockedA = std::max(0.0F, correctionA
            - ((ax - a.getX()) * dx + (ay - a.getY()) * dy));
    const float blockedB = std::max(0.0F, correctionB
            - ((b.getX() - bx) * dx + (b.getY() - by) * dy));
    if (bMovable) correctPlayerPosition(b, dx * blockedA, dy * blockedA);
    if (aMovable) correctPlayerPosition(a, -dx * blockedB, -dy * blockedB);
}

void resolvePlayerCollisions(
        Team& blueTeam,
        const std::array<PlayerPosition, Team::MAX_PLAYERS>& bluePrevious,
        Team& redTeam,
        const std::array<PlayerPosition, Team::MAX_PLAYERS>& redPrevious) {
    constexpr std::size_t CAPACITY = 2U * Team::MAX_PLAYERS;
    std::array<Player*, CAPACITY> players{};
    std::array<PlayerPosition, CAPACITY> previous{};
    std::array<bool, CAPACITY> movable{};
    std::size_t count = 0;
    for (std::size_t i = 0; i < blueTeam.getPlayerCount(); ++i, ++count) {
        players[count] = blueTeam.getPlayers()[i];
        previous[count] = bluePrevious[i];
    }
    for (std::size_t i = 0; i < redTeam.getPlayerCount(); ++i, ++count) {
        players[count] = redTeam.getPlayers()[i];
        previous[count] = redPrevious[i];
    }

    // Mobility is physical, not selection-based: moving humans and companions
    // share correction; a player stopped this step remains an obstacle.
    for (std::size_t i = 0; i < count; ++i) {
        movable[i] = players[i]->getX() != previous[i].x
                     || players[i]->getY() != previous[i].y;
    }

    constexpr int MAX_PASSES = 8;
    constexpr float OVERLAP_TOLERANCE = 0.0001F;
    for (int pass = 0; pass <= MAX_PASSES; ++pass) {
        std::array<PlayerPosition, CAPACITY> corrections{};
        bool overlapping = false;
        for (std::size_t i = 0; i < count; ++i) {
            for (std::size_t j = i + 1U; j < count; ++j) {
                if (!movable[i] && !movable[j]) continue;
                const float dx = players[j]->getX() - players[i]->getX();
                const float dy = players[j]->getY() - players[i]->getY();
                const float radius = players[i]->getRadius() + players[j]->getRadius();
                if (dx * dx + dy * dy >= (radius - OVERLAP_TOLERANCE)
                                                 * (radius - OVERLAP_TOLERANCE)) continue;
                overlapping = true;
                Player a = *players[i];
                Player b = *players[j];
                resolvePlayerPair(a, movable[i], previous[i],
                                   b, movable[j], previous[j]);
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
            if (movable[i]) correctPlayerPosition(*players[i], corrections[i].x, corrections[i].y);
        }
    }

    // A blocked cluster cannot push stationary teammates. Prefer the last valid
    // short-step positions to forcing a player through an obstacle or wall.
    bool previousValid = true;
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = i + 1U; j < count; ++j) {
            if (!movable[i] && !movable[j]) continue;
            const float dx = previous[j].x - previous[i].x;
            const float dy = previous[j].y - previous[i].y;
            const float radius = players[i]->getRadius() + players[j]->getRadius()
                                 - OVERLAP_TOLERANCE;
            if (dx * dx + dy * dy < radius * radius) previousValid = false;
        }
    }
    if (previousValid) {
        for (std::size_t i = 0; i < count; ++i) {
            if (movable[i]) {
                players[i]->setX(previous[i].x);
                players[i]->setY(previous[i].y);
            }
        }
    }
}

void resolveTeamBallCollisions(
        const Team& team, const std::array<PlayerPosition, Team::MAX_PLAYERS>& previous,
        Ball& ball, float deltaTime, bool& impulseApplied,
        const Player* protectedPlayer = nullptr) {
    for (std::size_t index = 0; index < team.getPlayerCount(); ++index) {
        const Player& player = *team.getPlayers()[index];
        const float velocityX = deltaTime > 0.0F
                ? (player.getX() - previous[index].x) / deltaTime : 0.0F;
        const float velocityY = deltaTime > 0.0F
                ? (player.getY() - previous[index].y) / deltaTime : 0.0F;
        if (resolvePlayerBallCollision(player, ball, velocityX, velocityY,
                                       !impulseApplied && &player != protectedPlayer)) {
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
                   double deltaTime, EstimatedPossession& possession, double& remainingTime) {
    if (remainingTime <= 0.0 || deltaTime <= 0.0) return false;
    deltaTime = std::min(deltaTime, remainingTime);
    // Consume even failed attempts once per frame, never once per substep.
    const bool blueKick = consumeKeyPress(isKeyPressed(window, BLUE_CONTROLS.kickKey),
                                          blueControlState.kickKeyWasPressed);
    const bool redKick = consumeKeyPress(isKeyPressed(window, RED_CONTROLS.kickKey),
                                         redControlState.kickKeyWasPressed);
    const bool bluePass = consumeKeyPress(isKeyPressed(window, BLUE_CONTROLS.passKey),
                                          blueControlState.passKeyWasPressed);
    const bool redPass = consumeKeyPress(isKeyPressed(window, RED_CONTROLS.passKey),
                                         redControlState.passKeyWasPressed);
    // Bound travel per step, including the existing impact speed, to avoid
    // skipping a player during an ordinary frame or a temporary frame delay.
    const float maximumSpeed = std::max(BALL_KICK_SPEED,
            std::hypot(ball.getVelocityX(), ball.getVelocityY())) + PLAYER_SPEED;
    const int steps = std::max(1, static_cast<int>(std::ceil(
            deltaTime * maximumSpeed / (ball.getRadius() * 0.5F))));
    const double nominalStep = deltaTime / steps;
    double processed = 0.0;
    for (int step = 0; step < steps && remainingTime > 0.0; ++step) {
        const double interval = std::min(remainingTime,
                step + 1 == steps ? deltaTime - processed : nominalStep);
        if (interval <= 0.0) break;
        const float stepTime = static_cast<float>(interval);
        if (stepTime <= 0.0F) break;
        const auto bluePrevious = capturePlayerPositions(blueTeam);
        const auto redPrevious = capturePlayerPositions(redTeam);
        handleTeamInput(window, blueTeam, blueControlState,
                        BLUE_CONTROLS, ball, stepTime);
        handleTeamInput(window, redTeam, redControlState,
                        RED_CONTROLS, ball, stepTime);
        possession = estimatePossession(blueTeam, redTeam, ball, possession);
        updateDefensiveMarking(blueTeam, redTeam, blueControlState, BLUE_INITIAL_POSITIONS, ball,
                               teamTacticalState(blueTeam.getId(), possession));
        updateDefensiveMarking(redTeam, blueTeam, redControlState, RED_INITIAL_POSITIONS, ball,
                               teamTacticalState(redTeam.getId(), possession));
        // Both teams observe the same pre-movement positions.
        const auto blueTargets = companionSupportTargets(blueTeam, blueControlState,
                BLUE_INITIAL_POSITIONS, ball, teamTacticalState(blueTeam.getId(), possession), &redTeam);
        const auto redTargets = companionSupportTargets(redTeam, redControlState,
                RED_INITIAL_POSITIONS, ball, teamTacticalState(redTeam.getId(), possession), &blueTeam);
        moveTeamCompanions(blueTeam, blueControlState, blueTargets, stepTime);
        moveTeamCompanions(redTeam, redControlState, redTargets, stepTime);
        resolvePlayerCollisions(blueTeam, bluePrevious, redTeam, redPrevious);
        const BallAction blueAction = step == 0 ? prepareBallAction(
                blueTeam, blueControlState, ball, blueKick, bluePass) : BallAction{};
        const BallAction redAction = step == 0 ? prepareBallAction(
                redTeam, redControlState, ball, redKick, redPass) : BallAction{};
        const bool rebounded = ball.update(stepTime, -FIELD_HALF_WIDTH, FIELD_HALF_WIDTH,
                    -FIELD_HALF_HEIGHT, FIELD_HALF_HEIGHT, GOAL_DEPTH, GOAL_HALF_HEIGHT);

        // A wall rebound ends the initial contact protection: return contacts
        // with the origin player are legitimate collisions again.
        if (rebounded) {
            blueControlState.initialContactPlayer = nullptr;
            redControlState.initialContactPlayer = nullptr;
        }
        const Player* protectedPlayer = protectedInitialContact(blueControlState, ball);
        const Player* redProtected = protectedInitialContact(redControlState, ball);
        if (protectedPlayer == nullptr) protectedPlayer = redProtected;

        // Fixed Blue/Red, registration order. At most one impulse per step;
        // extra passes correct positions only, including wall constraints.
        bool impulseApplied = false;
        for (int pass = 0; pass < 4; ++pass) {
            resolveTeamBallCollisions(blueTeam, bluePrevious, ball, stepTime,
                                      impulseApplied, protectedPlayer);
            resolveTeamBallCollisions(redTeam, redPrevious, ball, stepTime,
                                      impulseApplied, protectedPlayer);
            if (pass == 0 && impulseApplied) {
                blueControlState.initialContactPlayer = nullptr;
                redControlState.initialContactPlayer = nullptr;
            }
            impulseApplied = true;
            const bool contactRebound = ball.update(0.0F, -FIELD_HALF_WIDTH, FIELD_HALF_WIDTH,
                        -FIELD_HALF_HEIGHT, FIELD_HALF_HEIGHT, GOAL_DEPTH, GOAL_HALF_HEIGHT);
            if (contactRebound) {
                blueControlState.initialContactPlayer = nullptr;
                redControlState.initialContactPlayer = nullptr;
                protectedPlayer = nullptr;
            }
            if (isBallPositionClear(blueTeam, redTeam, ball)) {
                break;
            }
        }
        releaseTrappedBall(blueTeam, redTeam, ball);
        // Apply after this step's contact resolution so the originating
        // contact cannot overwrite a kick (35) or pass (25) with impact speed 20.
        const BallAction action = chooseBallAction(blueAction, redAction, ball);
        if (action.origin != nullptr) {
            const bool blue = action.origin->getTeam() == TeamId::Blue;
            TeamControlState& state = blue ? blueControlState : redControlState;
            bool executed = true;
            if (action.receiver != nullptr) {
                executed = applyPass(action, state, ball);
            } else {
                applyKick(window, *action.origin, blue ? BLUE_CONTROLS : RED_CONTROLS, ball);
            }
            if (executed) {
                // Keep the origin's identity even after a pass changes selection.
                blueControlState.initialContactPlayer = blue ? action.origin : nullptr;
                redControlState.initialContactPlayer = blue ? nullptr : action.origin;
                if (action.receiver != nullptr) {
                    updateDefensiveMarking(blue ? blueTeam : redTeam, blue ? redTeam : blueTeam,
                                           state, blue ? BLUE_INITIAL_POSITIONS : RED_INITIAL_POSITIONS,
                                           ball, teamTacticalState(action.origin->getTeam(), possession));
                }
            }
        }
        // Count only this simulated interval. A goal at its endpoint still
        // counts; Goal takes precedence, then leads to Finished if time is up.
        processed += interval;
        remainingTime = std::max(0.0, remainingTime - interval);
        if (remainingTime < TIME_ROUNDING_TOLERANCE) remainingTime = 0.0;
        if (updateScore(ball, blueTeam, redTeam)) {
            possession = EstimatedPossession::Neutral;
            blueControlState.marking = {};
            redControlState.marking = {};
            blueControlState.support = {};
            redControlState.support = {};
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

void resetPlay(Team& blueTeam, Team& redTeam, Ball& ball, EstimatedPossession& possession,
                TeamControlState& blueState, TeamControlState& redState) {
    resetControlledPlayer(blueState);
    resetControlledPlayer(redState);
    possession = EstimatedPossession::Neutral;
    resetPlayers(blueTeam, BLUE_INITIAL_POSITIONS);
    resetPlayers(redTeam, RED_INITIAL_POSITIONS);
    ball.setX(BALL_INITIAL_X);
    ball.setY(BALL_INITIAL_Y);
    ball.setVelocityX(BALL_INITIAL_VELOCITY_X);
    ball.setVelocityY(BALL_INITIAL_VELOCITY_Y);
}

void restartMatch(GLFWwindow* window, Team& blue, Team& red, Ball& ball,
                  EstimatedPossession& possession, TeamControlState& blueState,
                  TeamControlState& redState, MatchProgress& match) {
    resetPlay(blue, red, ball, possession, blueState, redState);
    blue.resetScore();
    red.resetScore();
    match = {};
    match.state = MatchState::Playing;
    match.enterWasPressed = isKeyPressed(window, GLFW_KEY_ENTER);
    match.menuWasPressed = isKeyPressed(window, GLFW_KEY_M);
    match.restartWasPressed = isKeyPressed(window, GLFW_KEY_R);
    syncTeamInputState(window, blueState, BLUE_CONTROLS);
    syncTeamInputState(window, redState, RED_CONTROLS);
}

void updateMatch(GLFWwindow* window, Team& blue, TeamControlState& blueState,
                 Team& red, TeamControlState& redState, Ball& ball,
                 EstimatedPossession& possession, MatchProgress& match, double deltaTime) {
    const bool restart = consumeKeyPress(isKeyPressed(window, GLFW_KEY_R), match.restartWasPressed);
    const bool play = consumeKeyPress(isKeyPressed(window, GLFW_KEY_ENTER), match.enterWasPressed);
    const bool menu = consumeKeyPress(isKeyPressed(window, GLFW_KEY_M), match.menuWasPressed);
    if (isKeyPressed(window, GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
    if (match.state == MatchState::Menu) {
        syncTeamInputState(window, blueState, BLUE_CONTROLS);
        syncTeamInputState(window, redState, RED_CONTROLS);
        if (play) restartMatch(window, blue, red, ball, possession, blueState, redState, match);
    } else if (match.state == MatchState::Playing) {
        if (updatePlaying(window, blue, blueState, red, redState, ball,
                          deltaTime, possession, match.remainingTime)) {
            match.state = MatchState::Goal;
            match.goalElapsed = 0.0;
            resetControlledPlayer(blueState);
            resetControlledPlayer(redState);
        } else if (match.remainingTime <= 0.0) {
            match.state = MatchState::Finished;
        }
    } else {
        syncTeamInputState(window, blueState, BLUE_CONTROLS);
        syncTeamInputState(window, redState, RED_CONTROLS);
        if (match.state == MatchState::Goal) {
            match.goalElapsed += std::max(0.0, deltaTime);
            if (match.goalElapsed + TIME_ROUNDING_TOLERANCE >= GOAL_STATE_DURATION) {
                if (match.remainingTime <= 0.0) {
                    match.state = MatchState::Finished;
                } else {
                    resetPlay(blue, red, ball, possession, blueState, redState);
                    match.state = MatchState::Playing;
                }
                match.goalElapsed = 0.0;
            }
        } else if (restart || menu) {
            // R wins a simultaneous R/M press; both edges are consumed.
            restartMatch(window, blue, red, ball, possession, blueState, redState, match);
            if (!restart) match.state = MatchState::Menu;
        }
    }
}

std::string formatMatchTime(double remainingTime) {
    const int seconds = remainingTime > 0.0
            ? std::max(1, static_cast<int>(std::ceil(remainingTime - TIME_ROUNDING_TOLERANCE))) : 0;
    const auto twoDigits = [](int value) {
        return (value < 10 ? std::string("0") : std::string()) + std::to_string(value);
    };
    return twoDigits(seconds / 60) + ":" + twoDigits(seconds % 60);
}

const char* matchResult(const Team& blue, const Team& red) {
    if (blue.getScore() > red.getScore()) return "BLUE WINS!";
    if (red.getScore() > blue.getScore()) return "RED WINS!";
    return "DRAW!";
}

void drawMatchClock(double remainingTime) {
    const std::string time = formatMatchTime(remainingTime);
    constexpr float SCALE = 0.6F;
    float x = 32.0F;
    glColor3f(1.0F, 1.0F, 1.0F);
    for (char c : time) {
        if (c == ':') {
            drawFilledRectangle(x + 0.7F, 36.1F, x + 1.1F, 36.5F);
            drawFilledRectangle(x + 0.7F, 37.5F, x + 1.1F, 37.9F);
        } else {
            drawDigit(c - '0', x, 35.5F, SCALE);
        }
        x += 3.8F * SCALE;
    }
}

// Shared 5x7 geometric lettering for menu and result; no external font.
std::array<unsigned char, 7> resultGlyph(char c) {
    switch (c) {
        case 'C': return {{14,17,16,16,16,17,14}};
        case 'F': return {{31,16,16,30,16,16,16}};
        case 'G': return {{14,17,16,23,17,17,14}};
        case 'H': return {{17,17,17,31,17,17,17}};
        case 'K': return {{17,18,20,24,20,18,17}};
        case 'M': return {{17,27,21,21,17,17,17}};
        case 'Q': return {{14,17,17,17,21,18,13}};
        case 'V': return {{17,17,17,17,17,10,4}};
        case 'X': return {{17,17,10,4,10,17,17}};
        case 'Y': return {{17,17,10,4,4,4,4}};
        case '2': return {{14,17,1,2,4,8,31}};
        case ':': return {{0,4,4,0,4,4,0}};
        case 'B': return {{30,17,17,30,17,17,30}};
        case 'L': return {{16,16,16,16,16,16,31}};
        case 'U': return {{17,17,17,17,17,17,14}};
        case 'E': return {{31,16,16,30,16,16,31}};
        case 'W': return {{17,17,17,21,21,21,10}};
        case 'I': return {{31,4,4,4,4,4,31}};
        case 'N': return {{17,25,25,21,19,19,17}};
        case 'S': return {{15,16,16,14,1,1,30}};
        case 'R': return {{30,17,17,30,20,18,17}};
        case 'D': return {{30,17,17,17,17,17,30}};
        case 'A': return {{14,17,17,31,17,17,17}};
        case 'P': return {{30,17,17,30,16,16,16}};
        case 'T': return {{31,4,4,4,4,4,4}};
        case 'O': return {{14,17,17,17,17,17,14}};
        case '!': return {{4,4,4,4,4,0,4}};
        default: return {{0,0,0,0,0,0,0}};
    }
}

void drawResultText(const std::string& text, float y, float scale, float centerX = 0.0F) {
    float x = centerX - (static_cast<float>(text.size()) * 6.0F - 1.0F) * scale / 2.0F;
    for (char c : text) {
        const auto glyph = resultGlyph(c);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((glyph[row] & (1U << (4 - column))) != 0U) {
                    const float left = x + column * scale;
                    const float bottom = y + (6 - row) * scale;
                    drawFilledRectangle(left, bottom, left + scale, bottom + scale);
                }
            }
        }
        x += 6.0F * scale;
    }
}

void drawMatchResult(const Team& blue, const Team& red) {
    glColor3f(0.03F, 0.12F, 0.05F);
    drawFilledRectangle(-42.0F, -23.0F, 42.0F, 14.0F);
    glColor3f(1.0F, 0.85F, 0.1F);
    drawResultText(matchResult(blue, red), 5.0F, 0.8F);
    glColor3f(1.0F, 1.0F, 1.0F);
    drawResultText("PRESS R TO RESTART", -8.0F, 0.6F);
    drawResultText("PRESS M FOR MENU", -14.0F, 0.6F);
    drawResultText("ESC TO EXIT", -20.0F, 0.5F);
    const float blueWidth = numberWidth(std::to_string(blue.getScore()), 0.8F);
    const float redWidth = numberWidth(std::to_string(red.getScore()), 0.8F);
    const float left = -(blueWidth + redWidth + 5.0F) / 2.0F;
    drawNumber(blue.getScore(), left, -1.5F, 0.8F);
    drawFilledRectangle(left + blueWidth + 1.5F, 0.2F, left + blueWidth + 3.5F, 0.7F);
    drawNumber(red.getScore(), left + blueWidth + 5.0F, -1.5F, 0.8F);
}

void drawMainMenu() {
    glColor3f(0.03F, 0.12F, 0.05F);
    drawFilledRectangle(-51.0F, -32.0F, 51.0F, 33.0F);
    glColor3f(1.0F, 0.85F, 0.1F);
    drawResultText("KICKOFF", 24.0F, 0.95F);
    glColor3f(1.0F, 1.0F, 1.0F);
    drawResultText("2 PLAYER FOOTBALL", 19.0F, 0.42F);
    drawResultText("PRESS ENTER TO PLAY", 13.0F, 0.5F);
    drawResultText("CONTROLS", 8.0F, 0.45F);
    glColor3f(0.1F, 0.3F, 0.95F);
    drawResultText("BLUE", 3.0F, 0.55F, -23.0F);
    glColor3f(0.9F, 0.15F, 0.15F);
    drawResultText("RED", 3.0F, 0.55F, 23.0F);
    glColor3f(1.0F, 1.0F, 1.0F);
    const std::array<const char*, 4> blue{{"MOVE: WASD", "SELECT: TAB", "SHOOT: SPACE", "PASS: Q"}};
    const std::array<const char*, 4> red{{"MOVE: ARROWS", "SELECT: ENTER", "SHOOT: RIGHT SHIFT", "PASS: RIGHT CTRL"}};
    for (std::size_t row = 0; row < blue.size(); ++row) {
        const float y = -3.0F - 5.0F * static_cast<float>(row);
        drawResultText(blue[row], y, 0.4F, -23.0F);
        drawResultText(red[row], y, 0.4F, 23.0F);
    }
    drawResultText("ESC TO EXIT", -27.0F, 0.45F);
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
    EstimatedPossession possession = EstimatedPossession::Neutral;
    MatchProgress match;

    double previousTime = glfwGetTime();

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        const double currentTime = glfwGetTime();
        const double deltaTime = currentTime - previousTime;
        previousTime = currentTime;

        updateMatch(window, blueTeam, blueControlState, redTeam, redControlState,
                    ball, possession, match, deltaTime);
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
        drawMatchClock(match.remainingTime);
        if (match.state == MatchState::Menu) {
            drawMainMenu();
        } else if (match.state == MatchState::Goal) {
            drawGoalMessage();
        } else if (match.state == MatchState::Finished) {
            drawMatchResult(blueTeam, redTeam);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
