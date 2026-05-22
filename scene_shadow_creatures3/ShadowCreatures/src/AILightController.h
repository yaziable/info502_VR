#pragma once
#include <glm/glm.hpp>
#include <vector>

class AILightController {
public:
    struct Light {
        glm::vec3 position, direction;
        float intensity, timer;
    };

    std::vector<Light> lights;
    glm::mat4 lightSpaceMatrix;

    AILightController();
    void update(float deltaTime);
    void spawnRandomLight();
    glm::vec3 getPosition() const;
    glm::mat4 getLightSpaceMatrix() const;
    float getIntensity() const;
    glm::vec3 getLightColor() const;
    
private:
    glm::mat4 createLightSpaceMatrix(glm::vec3 lightPos);
};