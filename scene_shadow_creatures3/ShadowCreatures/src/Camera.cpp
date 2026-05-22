#include "Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(2.5f), MouseSensitivity(0.1f), Zoom(45.0f) {
    Position = position;
    WorldUp = up;
    Yaw = -90.0f;
    Pitch = 0.0f;
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(Zoom), 1280.0f / 720.0f, 0.1f, 100.0f);
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD) Position += Front * velocity;
    if (direction == BACKWARD) Position -= Front * velocity;
    if (direction == LEFT) Position -= Right * velocity;
    if (direction == RIGHT) Position += Right * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    Yaw   += xoffset;
    Pitch += yoffset;
    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;
    updateCameraVectors();
}

void Camera::processMousePan(float xoffset, float yoffset) {
    float panSpeed = 0.035f;
    glm::vec3 flatFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
    Position -= Right * xoffset * panSpeed;
    Position += flatFront * yoffset * panSpeed;
}

void Camera::processMouseScroll(float yoffset) {
    float scrollSpeed = 1.8f;
    Position += Front * yoffset * scrollSpeed;
}

void Camera::update(float deltaTime) {}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}
