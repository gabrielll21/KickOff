#include "entities/Ball.h"
#include "entities/Player.h"
#include "entities/Team.h"

#include <GLFW/glfw3.h>

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

void updateControlledPlayerIndex(TeamControlState& state,
                                 std::size_t playerCount,
                                 bool switchKeyPressed) {
    if (switchKeyPressed && !state.switchKeyWasPressed && playerCount > 0U) {
        state.controlledPlayerIndex =
                (state.controlledPlayerIndex + 1U) % playerCount;
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
                     float deltaTime) {
    updateControlledPlayerIndex(state, team.getPlayerCount(),
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

void resolvePlayerBallCollision(const Player& player, Ball& ball) {
    const float deltaX = ball.getX() - player.getX();
    const float deltaY = ball.getY() - player.getY();
    const float minimumDistance = player.getRadius() + ball.getRadius();
    const float distanceSquared = deltaX * deltaX + deltaY * deltaY;

    if (distanceSquared > minimumDistance * minimumDistance) {
        return;
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
    ball.setVelocityX(directionX * BALL_IMPACT_SPEED);
    ball.setVelocityY(directionY * BALL_IMPACT_SPEED);
}

bool updatePlaying(GLFWwindow* window,
                   Team& blueTeam, TeamControlState& blueControlState,
                   Team& redTeam, TeamControlState& redControlState,
                   Ball& ball,
                   float deltaTime) {
    handleTeamInput(window, blueTeam, blueControlState,
                    BLUE_CONTROLS, deltaTime);
    handleTeamInput(window, redTeam, redControlState,
                    RED_CONTROLS, deltaTime);
    resolvePlayerBallCollision(
            getControlledPlayer(blueTeam, blueControlState), ball);
    resolvePlayerBallCollision(
            getControlledPlayer(redTeam, redControlState), ball);
    ball.update(deltaTime, -FIELD_HALF_WIDTH, FIELD_HALF_WIDTH,
                -FIELD_HALF_HEIGHT, FIELD_HALF_HEIGHT,
                GOAL_DEPTH, GOAL_HALF_HEIGHT);
    return updateScore(ball, blueTeam, redTeam);
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
