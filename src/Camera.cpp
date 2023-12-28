//
// Created by thomas on 28/12/23.
//

#include "Camera.h"

Camera::Camera()
{
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    fovy = 45.0f;
    aspect = 1.0f;
    near = 0.1f;
    far = 100.0f;
    projectionMatrix = glm::perspective(glm::radians(fovy), aspect, near, far);
}
Camera::Camera(float x, float y, float z)
{
    position = glm::vec3(x, y, z);
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    fovy = 45.0f;
    aspect = 1.0f;
    near = 0.1f;
    far = 100.0f;
    projectionMatrix = glm::perspective(glm::radians(fovy), aspect, near, far);

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
