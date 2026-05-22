#include "ShadowCreature.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>

float ShadowCreature::growthSpeed = 1.0f;

ShadowCreature::ShadowCreature(glm::vec3 pos)
    : position(pos), size(0.1f), isMorphed(false), isCrystal(false) {
    initBlobMesh();
    initParticleMesh();
    initPolyMesh();
}

//Rajout de l'interaction particule - lumière
void ShadowCreature::update(float deltaTime, GLuint shadowMap,
                             glm::vec3 lightPos, float sunIntensity,
                             glm::vec3 lightColor, bool inShade) {
    if (isMorphed) {
        updateParticles(deltaTime, sunIntensity, lightColor);
        return;
    }

    // Jour → rétrécit, Nuit → grandit
    const float maxCreatureSize = 1.0f;

    if (inShade) {
        size += growthSpeed * deltaTime * 0.08f;
    } else {
        size -= growthSpeed * deltaTime * 0.2f;
    }

    //if (size < 0.05f) size = 0.05f;

    if (size >= maxCreatureSize) {
        size = maxCreatureSize;
        morphToCreature(sunIntensity, lightColor);
    }
    if (size < 0.0f) size = 0.0f;

    updateParticles(deltaTime, sunIntensity, lightColor);
}

void ShadowCreature::render(Shader& shader) {
    glm::mat4 model = glm::mat4(1.0f);

    if (isMorphed) {
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(size * (isCrystal ? 0.72f : 0.5f)));
        shader.setInt("materialType", isCrystal ? 7 : 4);
        shader.setVec3("objectColor",
                       isCrystal ? glm::vec3(0.36f, 0.92f, 1.0f)
                                 : glm::vec3(0.08f, 0.07f, 0.12f));
        shader.setMat4("model", model);
        shader.setInt("isParticle", 0);

        glBindVertexArray(polyVAO);
        glDrawElements(GL_TRIANGLES, polyIndexCount,
                       GL_UNSIGNED_INT, 0);

        if (isCrystal) {
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0f, -1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            shader.setVec3("objectColor", glm::vec3(0.95f, 1.0f, 1.0f));
            glDrawElements(GL_TRIANGLES, polyIndexCount,
                           GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_POLYGON_OFFSET_LINE);
        }
    } else {
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(size));
        shader.setInt("materialType", 4);
        shader.setVec3("objectColor", glm::vec3(0.5f, 0.5f, 0.5f));
        shader.setMat4("model", model);
        shader.setInt("isParticle", 0);

        glBindVertexArray(VAO);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, indexCount,
                       GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    renderParticles(shader);
}

void ShadowCreature::renderShadow(Shader& shader) {
    if (!isMorphed) return;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(size * 0.5f));
    shader.setMat4("model", model);

    glBindVertexArray(polyVAO);
    glDrawElements(GL_TRIANGLES, polyIndexCount,
                   GL_UNSIGNED_INT, 0);
}

void ShadowCreature::spawnParticles(float sunIntensity, glm::vec3 lightColor, float speed) {
    for (int i = 0; i < 40; i++) {
        Particle p;
        p.position = position;

        float vx = (rand() % 200 - 100) / 100.0f;
        float vy = (rand() % 200)        / 100.0f;
        float vz = (rand() % 200 - 100) / 100.0f;
        p.velocity = glm::vec3(vx, vy, vz) * speed;

        //p.color = glm::vec4(
          //  0.9f + (rand() % 10) / 100.0f,
            //0.3f + (rand() % 30) / 100.0f,
          //  0.0f,
            //1.0f
       // );
        //La nuit les particles vont prendre la couleur bleues/violettes
        //Le jour, les particules vont être teintées de la couleur du soleil
        glm::vec3 nightColor(0.3f, 0.2f, 0.9f);
        glm::vec3 dayColor(
            0.9f + (rand() % 10) / 100.0f,
            0.3f + (rand() % 30) / 100.0f,
            0.0f
        );
        glm::vec3 finalColor = glm::mix(nightColor, dayColor * lightColor, sunIntensity);
        p.baseColor = finalColor;
        p.color = glm::vec4(finalColor, 1.0f);

        p.life = 0.5f + (rand() % 100) / 100.0f;
        p.size = 0.05f + (rand() % 15) / 100.0f;
        particles.push_back(p);
    }
}

void ShadowCreature::updateParticles(float deltaTime, float sunIntensity, glm::vec3 lightColor) {
    glm::vec3 nightColor(0.3f, 0.2f, 0.9f);

    for (auto& p : particles) {
        p.position += p.velocity * deltaTime;
        p.velocity.y -= 9.8f * deltaTime;
        p.life -= deltaTime;
        p.color.a = glm::max(0.0f, p.life);

        // Interaction particule-lumière : recalcul couleur selon lumière actuelle
        glm::vec3 updatedColor = glm::mix(nightColor, p.baseColor * lightColor, sunIntensity);
        p.color = glm::vec4(updatedColor, p.color.a);
    }

    // --- détection de collision O(n²) simple ---
    float particleRadius = 0.3f;   // rayon approximatif des particules
    float minDist = 2.0f * particleRadius;
    float minDist2 = minDist * minDist;

    for (size_t i = 0; i < particles.size(); ++i) {
    for (size_t j = i + 1; j < particles.size(); ++j) {
        glm::vec3 diff = particles[j].position - particles[i].position;
        float dist2 = glm::dot(diff, diff);

        if (dist2 < minDist2 && dist2 > 1e-6f) {
            // Axe de collision normalisé
            glm::vec3 n = diff / std::sqrt(dist2);

            // Vitesse relative projetée sur l'axe
            float impulse = glm::dot(
                particles[i].velocity - particles[j].velocity, n);

            // N'appliquer que si les particules se rapprochent
            if (impulse > 0.0f) {
                float restitution = 0.6f;   // 0=mou, 1=parfaitement élastique
                particles[i].velocity -= (1.0f + restitution) * 0.5f * impulse * n;
                particles[j].velocity += (1.0f + restitution) * 0.5f * impulse * n;
            }

            // Correction de pénétration (séparer les particules)
            float overlap = minDist - std::sqrt(dist2);
            particles[i].position -= 0.5f * overlap * n;
            particles[j].position += 0.5f * overlap * n;
        }
    }
    }


    particles.erase(
        std::remove_if(
            particles.begin(),
            particles.end(),
            [](const Particle& p) { return p.life <= 0.0f; }
        ),
        particles.end()
    );
}

void ShadowCreature::renderParticles(Shader& shader) {
    if (particles.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    shader.setInt("isParticle", 1);
    glBindVertexArray(particleVAO);

    for (const auto& p : particles) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, p.position);
        model = glm::scale(model, glm::vec3(p.size));

        shader.setMat4("model", model);
        shader.setVec3("particleColor", glm::vec3(p.color));
        shader.setFloat("particleAlpha", p.color.a);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

// Modifié pour l'interaction entre particule
void ShadowCreature::morphToCreature(float sunIntensity, glm::vec3 lightColor) {
    isMorphed = true;
    isCrystal = (rand() % 3) == 0;
    // Explosion initiale : vitesse élevée pour un effet burst visible
    spawnParticles(sunIntensity, lightColor, 3.0f);
    std::cout << "Creature Morphed! Particles spawned: "
             << particles.size() << std::endl;
}

void ShadowCreature::initPolyMesh() {
    float t = (1.0f + sqrt(5.0f)) / 2.0f;

    std::vector<glm::vec3> positions = {
        {-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0},
        {0, -1, t}, {0, 1, t}, {0, -1, -t}, {0, 1, -t},
        {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}
    };

    std::vector<unsigned int> idx = {
        0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11,
        1,5,9,  5,11,4, 11,10,2, 10,7,6, 7,1,8,
        3,9,4,  3,4,2,  3,2,6,  3,6,8,  3,8,9,
        4,9,5,  2,4,11, 6,2,10, 8,6,7,  9,8,1
    };

    std::vector<float> verts;
    verts.reserve(positions.size() * 8);
    for (const glm::vec3& pos : positions) {
        glm::vec3 normal = glm::normalize(pos);
        float u = 0.5f + std::atan2(normal.z, normal.x) / (2.0f * 3.14159265359f);
        float v = 0.5f - std::asin(normal.y) / 3.14159265359f;

        verts.push_back(pos.x);
        verts.push_back(pos.y);
        verts.push_back(pos.z);
        verts.push_back(normal.x);
        verts.push_back(normal.y);
        verts.push_back(normal.z);
        verts.push_back(u);
        verts.push_back(v);
    }

    polyIndexCount = idx.size();

    glGenVertexArrays(1, &polyVAO);
    glGenBuffers(1, &polyVBO);
    glGenBuffers(1, &polyEBO);

    glBindVertexArray(polyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, polyVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 verts.size() * sizeof(float),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, polyEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 idx.size() * sizeof(unsigned int),
                 idx.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void ShadowCreature::initParticleMesh() {
    float quadVertices[] = {
        -0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f
    };

    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);

    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices),
                 quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void ShadowCreature::initBlobMesh() {
    vertices.clear();
    std::vector<unsigned int> indices;
    const unsigned int X_SEGMENTS = 32;
    const unsigned int Y_SEGMENTS = 32;
    const float PI = 3.14159265359f;

    for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;

            float xPos = xSegment - 0.5f;
            float zPos = ySegment - 0.5f;

            float distanceToCenter = std::sqrt(xPos*xPos + zPos*zPos);
            float yPos = std::cos(distanceToCenter * PI) * 0.5f;
            if (distanceToCenter > 0.5f) yPos = 0.0f;

            float noise = std::sin(xPos*25.0f)
                        * std::cos(zPos*25.0f) * 0.05f;
            yPos += noise;

            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
            vertices.push_back(xSegment);
            vertices.push_back(ySegment);
        }
    }

    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            indices.push_back((y+1)*(X_SEGMENTS+1)+x);
            indices.push_back(y*(X_SEGMENTS+1)+x);
            indices.push_back(y*(X_SEGMENTS+1)+x+1);
            indices.push_back((y+1)*(X_SEGMENTS+1)+x);
            indices.push_back(y*(X_SEGMENTS+1)+x+1);
            indices.push_back((y+1)*(X_SEGMENTS+1)+x+1);
        }
    }
    indexCount = indices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
}

float ShadowCreature::calculateShadowFactor(GLuint shadowMap,
                                             glm::vec3 lightPos) {
    return 0.0f;
}
