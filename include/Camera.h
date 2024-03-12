//
// Created by thomas on 28/12/23.
//
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef DISS_CAMERA_H
#define DISS_CAMERA_H


class Camera
{
public:
    Camera();
    Camera(float x, float y, float z);
    void moveForward(float distance);
    void moveBackward(float distance);
    void moveLeft(float distance);
    void moveRight(float distance);
    void moveUp(float distance);
    void moveDown(float distance);
    void rotateLeft(float angle);
    void rotateRight(float angle);
    void rotateUp(float angle);
    void rotateDown(float angle);
    void getInput(GLFWwindow* window);
    float getFocalX();
    float getFocalY();
    float getTanFovy();
    float getTanFovx();

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    glm::mat3 getRotationMatrix();

    void setWidthHeight(int width, int height);
    void setPosition(float x, float y, float z);
    void setRotation(float x, float y, float z);
    void setFovy(float fovy);
    void update();

private:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 rotationMatrix;

    float fovy;
    float aspect;
    float near;
    float far;
    int width = 1024;
public:
    int getWidth() const;

    int getHeight() const;

private:
    int height = 512;



};


#endif //DISS_CAMERA_H
