#include "entities/Ball.h"
#include "entities/Player.h"

#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

namespace {
constexpr double WORLD_WIDTH = 120.0;
constexpr double WORLD_HEIGHT = 80.0;
constexpr float FIELD_HALF_WIDTH = 52.5F;
constexpr float PLAYER_INITIAL_X = -20.0F;
constexpr float PLAYER_INITIAL_Y = 0.0F;
constexpr float PLAYER_SPEED = 12.0F;
constexpr float PLAYER_RADIUS = 2.0F;
constexpr float BALL_INITIAL_X = 0.0F;
constexpr float BALL_INITIAL_Y = 0.0F;
constexpr float BALL_INITIAL_VELOCITY_X = 14.0F;
constexpr float BALL_INITIAL_VELOCITY_Y = 9.0F;
constexpr float BALL_RADIUS = 1.0F;
constexpr float FIELD_HALF_HEIGHT = 34.0F;
constexpr double PI = 3.14159265358979323846;

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

void handlePlayerInput(GLFWwindow* window, Player& player, float deltaTime) {
    float directionX = 0.0F;
    float directionY = 0.0F;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        directionY += 1.0F;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        directionY -= 1.0F;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        directionX -= 1.0F;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
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

void drawPlayer(const Player& player) {
    constexpr int SEGMENTS = 48;

    if (player.getTeam() == Team::Blue) {
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

    drawRectangle(-FIELD_HALF_WIDTH - 2.44F, -3.66F,
                  -FIELD_HALF_WIDTH, 3.66F);
    drawRectangle(FIELD_HALF_WIDTH, -3.66F,
                  FIELD_HALF_WIDTH + 2.44F, 3.66F);
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

    Player player(PLAYER_INITIAL_X, PLAYER_INITIAL_Y,
                  PLAYER_SPEED, PLAYER_RADIUS, Team::Blue);

    Ball ball(BALL_INITIAL_X, BALL_INITIAL_Y,
              BALL_INITIAL_VELOCITY_X, BALL_INITIAL_VELOCITY_Y,
              BALL_RADIUS);

    double previousTime = glfwGetTime();

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        handlePlayerInput(window, player, deltaTime);
        ball.update(deltaTime, -FIELD_HALF_WIDTH, FIELD_HALF_WIDTH,
                    -FIELD_HALF_HEIGHT, FIELD_HALF_HEIGHT);
        glClearColor(0.03F, 0.12F, 0.05F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();
        drawField();
        drawPlayer(player);
        drawBall(ball);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
