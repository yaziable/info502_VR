#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
public:
    glm::vec3 Position, Front, Up, Right, WorldUp;
    float Yaw, Pitch, MovementSpeed, MouseSensitivity, Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    void processKeyboard(Camera_Movement direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset);
    void processMousePan(float xoffset, float yoffset);
    void processMouseScroll(float yoffset);
    void update(float deltaTime);

    glm::vec3 getPosition() const { return Position; }
private:
    void updateCameraVectors();
};
