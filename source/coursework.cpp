#include <iostream>
#include <cmath>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/glew.h>

#include <common/shader.hpp>
#include <common/model.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

glm::vec3 cameraPos = glm::vec3(0.0f, -9.4f, -20.0f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.1f, 1.0f));
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 512.0f, lastY = 384.0f;
bool firstMouse = true;

float cameraSpeed = 0.03f;
glm::vec3 tablePos = glm::vec3(0.0f, -9.925f, 0.0f);
float tableTriggerRadius = 1.5f;

glm::vec3 eyeballPos = glm::vec3(0.0f, -6.0f, 0.0f);
float eyeballRadius = 1.0f;

GLuint roomVAO, roomVBO;

void processInput(GLFWwindow* window);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void setupRoom();
void renderRoom(GLuint shaderID, const glm::mat4& view, const glm::mat4& projection, bool siren);

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Eyeball Viewer", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = LoadShaders("vertexShader.glsl", "fragmentShader.glsl");
    if (!shaderID) return -1;

    Model eyeball("../assets/objects/eyeball.obj");
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 100.0f);

    setupRoom();

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderID);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        GLint mvpLoc = glGetUniformLocation(shaderID, "MVP");
        GLint colorLoc = glGetUniformLocation(shaderID, "baseColor");

        float tableDist = glm::length(glm::vec2(cameraPos.x, cameraPos.z) - glm::vec2(tablePos.x, tablePos.z));
        bool siren = tableDist < tableTriggerRadius;

        renderRoom(shaderID, view, projection, siren);

        glm::vec3 bulbColor = siren ? (fmod(glfwGetTime(), 1.0f) < 0.5f ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1)) : glm::vec3(1, 1, 0);

        for (float xOffset : {-0.4f, 0.4f}) {
            glm::mat4 bulbModel = glm::translate(glm::mat4(1.0f), glm::vec3(xOffset, 9.8f, 0.0f));
            bulbModel = glm::scale(bulbModel, glm::vec3(0.1f));
            glm::mat4 bulbMVP = projection * view * bulbModel;
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(bulbMVP));
            glUniform3fv(colorLoc, 1, glm::value_ptr(bulbColor));
            glBindVertexArray(roomVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glm::mat4 standModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -9.925f, 0.0f));
        standModel = glm::scale(standModel, glm::vec3(0.15f));
        glm::mat4 standMVP = projection * view * standModel;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(standMVP));
        glUniform3f(colorLoc, 0.5f, 0.3f, 0.1f);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), eyeballPos);
        model = glm::scale(model, glm::vec3(0.8f));
        glm::mat4 MVP = projection * view * model;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
        eyeball.draw(shaderID);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    float currentSpeed = cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        currentSpeed *= 2.0f;

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
    glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 next = cameraPos;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) next += flatFront * currentSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) next -= flatFront * currentSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) next -= right * currentSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) next += right * currentSpeed;

    next.y = 0.3f;

    bool withinBounds =
        next.x > -29.5f && next.x < 29.5f &&
        next.z > -29.5f && next.z < 29.5f;

    float horizontalDist = glm::length(glm::vec2(next.x, next.z) - glm::vec2(eyeballPos.x, eyeballPos.z));
    bool collision = horizontalDist < (eyeballRadius + 1.5f);

    if (withinBounds && !collision)
        cameraPos = next;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = float(xpos - lastX) * 0.1f;
    float yoffset = float(lastY - ypos) * 0.1f;
    lastX = float(xpos);
    lastY = float(ypos);

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void setupRoom() {
    float sx = 30.0f, sy = 10.0f, sz = 30.0f;
    float vertices[] = {
        -sx,-sy,-sz, sx,sy,-sz, sx,-sy,-sz, sx,sy,-sz, -sx,-sy,-sz, -sx,sy,-sz,
        -sx,-sy, sz, sx,-sy, sz, sx, sy, sz, sx, sy, sz, -sx, sy, sz, -sx,-sy, sz,
        -sx,-sy,-sz, -sx,-sy, sz, -sx, sy, sz, -sx, sy, sz, -sx, sy,-sz, -sx,-sy,-sz,
         sx,-sy,-sz,  sx, sy,-sz,  sx, sy, sz,  sx, sy, sz,  sx,-sy, sz,  sx,-sy,-sz,
        -sx,-sy,-sz, sx,-sy,-sz, sx,-sy, sz, sx,-sy, sz, -sx,-sy, sz, -sx,-sy,-sz,
        -sx, sy,-sz, -sx, sy, sz,  sx, sy, sz,  sx, sy, sz,  sx, sy,-sz, -sx, sy,-sz
    };
    glGenVertexArrays(1, &roomVAO);
    glGenBuffers(1, &roomVBO);
    glBindVertexArray(roomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void renderRoom(GLuint shaderID, const glm::mat4& view, const glm::mat4& projection, bool siren) {
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 MVP = projection * view * model;
    GLuint mvpLoc = glGetUniformLocation(shaderID, "MVP");
    GLuint colorLoc = glGetUniformLocation(shaderID, "baseColor");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));
    glBindVertexArray(roomVAO);

    glm::vec3 baseColors[6] = {
        {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f}, {0.4f, 0.7f, 1.0f}, {1.0f, 0.0f, 1.0f}
    };

    glm::vec3 tint = siren ? (fmod(glfwGetTime(), 1.0f) < 0.5f ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1)) : glm::vec3(1);

    for (int i = 0; i < 6; ++i) {
        glm::vec3 finalColor = siren ? tint : baseColors[i];
        glUniform3fv(colorLoc, 1, glm::value_ptr(finalColor));
        glDrawArrays(GL_TRIANGLES, i * 6, 6);
    }
}
