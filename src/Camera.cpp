//
// Created by thomas on 28/12/23.
//

#include "Camera.h"

Camera::Camera()
{
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    fovy = 60.0f;
    aspect = 1.0f;
    near = 0.1f;
    far = 100.0f;
    projectionMatrix = glm::perspective(glm::radians(fovy), aspect, near, far);
    update();

}
Camera::Camera(float x, float y, float z)
{
    position = glm::vec3(x, y, z);
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    fovy = 60.0f;
    aspect = 1.0f;
    near = 0.1f;
    far = 1000.0f;
    projectionMatrix = glm::perspective(glm::radians(fovy), aspect, near, far);
    update();

}


glm::mat4 Camera::getProjectionMatrix()
{
    return projectionMatrix;
}
glm::mat4 Camera::getViewMatrix()
{
    return viewMatrix;
}
glm::mat3 Camera::getRotationMatrix()
{
    //convert rotation matrix to mat3
    return glm::mat3(rotationMatrix[0][0], rotationMatrix[0][1], rotationMatrix[0][2],
                     rotationMatrix[1][0], rotationMatrix[1][1], rotationMatrix[1][2],
                     rotationMatrix[2][0], rotationMatrix[2][1], rotationMatrix[2][2]);
}

void Camera::setWidthHeight(int width, int height)
{
    this->width = width;
    this->height = height;
    aspect = (float)width / (float)height;
    projectionMatrix = glm::perspective(glm::radians(fovy), aspect, near, far);
}

void Camera::update()
{
    glm::mat4 rotationMatrixX = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationMatrixY = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotationMatrixZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    rotationMatrix = glm::mat3(rotationMatrixX * rotationMatrixY * rotationMatrixZ);
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
    viewMatrix = rotationMatrix * translationMatrix;
}

int Camera::getWidth() const
{
    return width;
}

int Camera::getHeight() const
{
    return height;
}

void Camera::getInput(GLFWwindow* window)
{
    //if w press down this frame, move forward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        moveForward(0.1f);
    }
    //if s press down this frame, move backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        moveBackward(0.1f);
    }
    //if a press down this frame, move left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        moveLeft(0.1f);
    }
    //if d press down this frame, move right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        moveRight(0.1f);
    }
    //if space press down this frame, move up
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        moveUp(0.1f);
    }
    //if left shift press down this frame, move down
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        moveDown(0.1f);
    }
    //if left press down this frame, rotate left
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        rotateLeft(0.1f);
    }
    //if right press down this frame, rotate right
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        rotateRight(0.1f);
    }
    //if up press down this frame, rotate up
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        rotateUp(0.1f);
    }
    //if down press down this frame, rotate down
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        rotateDown(0.1f);
    }
}

void Camera::moveForward(float distance)
{
    //move forward in the direction of the camera
    position = position + glm::vec3((-rotationMatrix[2] * distance).x, (-rotationMatrix[2] * distance).y, (-rotationMatrix[2] * distance).z);
    update();
}

void Camera::moveBackward(float distance)
{
    moveForward(-distance);
}

void Camera::moveLeft(float distance)
{
    //move left in the direction of the camera
    position = position + glm::vec3((rotationMatrix[0] * distance).x, (rotationMatrix[0] * distance).y, (rotationMatrix[0] * distance).z);
    update();
}

void Camera::moveRight(float distance)
{
    moveLeft(-distance);
}

void Camera::moveUp(float distance)
{
    //move up in the direction of the camera
    position = position + glm::vec3((rotationMatrix[1] * distance).x, (rotationMatrix[1] * distance).y, (rotationMatrix[1] * distance).z);
    update();
}

void Camera::moveDown(float distance)
{
    moveUp(-distance);
}

void Camera::rotateRight(float angle)
{
    //rotate left
    rotation.y += angle;
    update();
}

void Camera::rotateLeft(float angle)
{
    rotateRight(-angle);
}

void Camera::rotateUp(float angle)
{
    //rotate up
    rotation.x += angle;
    update();
}

void Camera::rotateDown(float angle)
{
    rotateUp(-angle);
}