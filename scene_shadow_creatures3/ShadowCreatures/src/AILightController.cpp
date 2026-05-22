#include "AILightController.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cmath>

AILightController::AILightController() {
    spawnRandomLight();
}

void AILightController::update(float deltaTime) {
    if (lights.empty()) spawnRandomLight();
    // Force exactly one sun (removes extra lights if the user clicks "Spawn Light")
    if (lights.size() > 1) lights.resize(1); 

    float time = (float)glfwGetTime();
    // Keep daylight long, but make sunset/night pass quickly.
    float cycleDuration = 52.0f;
    float daylightPart = 0.82f;
    float phase = std::fmod(time / cycleDuration, 1.0f);
    float angle;
    if (phase < daylightPart) {
        angle = 3.14159265359f * (phase / daylightPart);
    } else {
        angle = 3.14159265359f
              + 3.14159265359f * ((phase - daylightPart) / (1.0f - daylightPart));
    }
    float orbitRadius = 45.0f;
    lights[0].position = glm::vec3(
        std::cos(angle) * orbitRadius,
        std::sin(angle) * 30.0f,
        -18.0f
    );
    
    // Make the sun always point down toward the center of the scene
    lights[0].direction = glm::normalize(glm::vec3(0.0f) - lights[0].position);
    lights[0].intensity = getIntensity();
    
    if (!lights.empty()) {
        lightSpaceMatrix = createLightSpaceMatrix(lights[0].position);
    }
}

void AILightController::spawnRandomLight() {
    Light l;
    l.position = glm::vec3((rand() % 20) - 10, 10.0f, (rand() % 20) - 10);
    l.direction = glm::normalize(glm::vec3((rand() % 10) - 5, 0, (rand() % 10) - 5));
    l.intensity = 1.0f;
    l.timer = 10.0f + (rand() % 10);
    lights.push_back(l);
}

glm::vec3 AILightController::getPosition() const {
    if (lights.empty()) return glm::vec3(0, 10, 0);
    return lights[0].position;
}

glm::mat4 AILightController::getLightSpaceMatrix() const {
    return lightSpaceMatrix;
}

float AILightController::getIntensity() const {
    if (lights.empty()) return 1.0f;
    float heightFactor = glm::clamp((lights[0].position.y + 4.0f) / 34.0f, 0.08f, 1.0f);
    return heightFactor;
}

glm::vec3 AILightController::getLightColor() const {
    float intensity = getIntensity();
    glm::vec3 sunriseColor(1.0f, 0.48f, 0.20f);
    glm::vec3 noonColor(1.0f, 0.96f, 0.82f);
    return glm::mix(sunriseColor, noonColor, glm::smoothstep(0.35f, 0.85f, intensity));
}

glm::mat4 AILightController::createLightSpaceMatrix(glm::vec3 lightPos) {
    float near_plane = 1.0f, far_plane = 90.0f;
    glm::mat4 lightProjection = glm::ortho(-42.0f, 42.0f, -42.0f, 42.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    return lightProjection * lightView;
}
