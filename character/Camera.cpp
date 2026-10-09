#include "Camera.h"

Camera::Camera()
{
    // Initial camera position.
    position = glm::vec3(0.0f, 0.0f, 3.0f);

    // Camera looks toward negative Z.
    front = glm::vec3(0.0f, 0.0f, -1.0f);

    // Positive Y is the up direction.
    up = glm::vec3(0.0f, 1.0f, 0.0f);

    speed = 2.5f;
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(
        position,
        position + front,
        up
    );
}

void Camera::moveForward(float deltaTime)
{
    position += front * speed * deltaTime;
}

void Camera::moveBackward(float deltaTime)
{
    position -= front * speed * deltaTime;
}

void Camera::moveLeft(float deltaTime)
{
    glm::vec3 right =
        glm::normalize(glm::cross(front, up));

    position -= right * speed * deltaTime;
}

void Camera::moveRight(float deltaTime)
{
    glm::vec3 right =
		glm::normalize(glm::cross(front, up)); // Right vector is perpendicular to front and up. consider rotation of camera

    position += right * speed * deltaTime;
}