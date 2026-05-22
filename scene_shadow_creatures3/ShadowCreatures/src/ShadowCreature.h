#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    glm::vec3 baseColor;  // couleur de référence pour l'interaction lumière
    float     life;
    float     size;
};

class ShadowCreature {
public:
    static float growthSpeed;
    glm::vec3    position;
    glm::vec3    velocity;
    float        size;
    float        growthTimer;
    float        shadowCoverage;
    bool         isMorphed;
    bool         isCrystal;

    std::vector<Particle> particles;

    ShadowCreature(glm::vec3 pos);
    void update(float deltaTime, GLuint shadowMap,
            glm::vec3 lightPos, float sunIntensity,
            glm::vec3 lightColor = glm::vec3(1.0f),
            bool inShade = false);
    void render(class Shader& shader);
    void renderShadow(class Shader& shader);

private:
    // Mesh blob
    GLuint VAO, VBO, EBO;
    std::vector<float> vertices;
    unsigned int indexCount;

    // Mesh particules
    GLuint particleVAO, particleVBO;

    // Mesh polygone (icosaèdre)
    GLuint polyVAO, polyVBO, polyEBO;
    unsigned int polyIndexCount;

    void initBlobMesh();
    void initParticleMesh();
    void initPolyMesh();
    float calculateShadowFactor(GLuint shadowMap, glm::vec3 lightPos);
    void morphToCreature(float sunIntensity, glm::vec3 lightColor);
    //void spawnParticles(); // modifié
    void spawnParticles(float sunIntensity, glm::vec3 lightColor, float speed = 0.8f);
    void updateParticles(float deltaTime, float sunIntensity = 0.5f, glm::vec3 lightColor = glm::vec3(1.0f));
    void renderParticles(class Shader& shader);
};
