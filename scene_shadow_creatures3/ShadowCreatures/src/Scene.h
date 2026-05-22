#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include <vector>
#include <cmath>
#include <cstdlib>

class Scene {
public:
    Scene() {
        std::vector<float> vertices;
        int gridSize = 120;
        float size = 90.0f;
        
        for (int z = 0; z <= gridSize; ++z) {
            for (int x = 0; x <= gridSize; ++x) {
                float xPos = (float)x / gridSize * size - size / 2.0f;
                float zPos = (float)z / gridSize * size - size / 2.0f;
                float yPos = terrainHeight(xPos, zPos);
                glm::vec3 normal = terrainNormal(xPos, zPos);
                
                vertices.push_back(xPos);
                vertices.push_back(yPos);
                vertices.push_back(zPos);
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
                vertices.push_back((float)x / gridSize);
                vertices.push_back((float)z / gridSize);
            }
        }
        
        for (int z = 0; z < gridSize; ++z) {
            for (int x = 0; x < gridSize; ++x) {
                int topLeft = z * (gridSize + 1) + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * (gridSize + 1) + x;
                int bottomRight = bottomLeft + 1;
                
                float avgY = (vertices[topLeft*8 + 1] + vertices[topRight*8 + 1] + vertices[bottomLeft*8 + 1] + vertices[bottomRight*8 + 1]) / 4.0f;

                if (avgY > 8.5f) {
                    indicesSnow.push_back(topLeft); indicesSnow.push_back(bottomLeft); indicesSnow.push_back(topRight);
                    indicesSnow.push_back(topRight); indicesSnow.push_back(bottomLeft); indicesSnow.push_back(bottomRight);
                } else if (avgY > 3.2f) {
                    indicesBrown.push_back(topLeft); indicesBrown.push_back(bottomLeft); indicesBrown.push_back(topRight);
                    indicesBrown.push_back(topRight); indicesBrown.push_back(bottomLeft); indicesBrown.push_back(bottomRight);
                } else {
                    indicesGreen.push_back(topLeft); indicesGreen.push_back(bottomLeft); indicesGreen.push_back(topRight);
                    indicesGreen.push_back(topRight); indicesGreen.push_back(bottomLeft); indicesGreen.push_back(bottomRight);
                }
            }
        }

        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glGenBuffers(1, &eboGreen);
        glGenBuffers(1, &eboBrown);
        glGenBuffers(1, &eboSnow);

        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboGreen); glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesGreen.size() * sizeof(unsigned int), indicesGreen.data(), GL_STATIC_DRAW); countGreen = indicesGreen.size();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboBrown); glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesBrown.size() * sizeof(unsigned int), indicesBrown.data(), GL_STATIC_DRAW); countBrown = indicesBrown.size();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboSnow); glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSnow.size() * sizeof(unsigned int), indicesSnow.data(), GL_STATIC_DRAW); countSnow = indicesSnow.size();
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindVertexArray(0);

        initSunMesh();
        initTerrainTextures();
        initTreeMeshes();
        populateTrees();
        initTreeInstances();
        initSkybox();
    }

    void render(const Shader& shader) {
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        shader.setInt("isParticle", 0);
        shader.setInt("useInstancing", 0);
        shader.setInt("grassTexture", 1);
        shader.setInt("rockTexture", 2);
        shader.setInt("snowTexture", 3);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, rockTexture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, snowTexture);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(planeVAO);
            
            // Draw Green Ground
            shader.setInt("materialType", 0);
            shader.setVec3("objectColor", glm::vec3(0.18f, 0.48f, 0.16f));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboGreen); glDrawElements(GL_TRIANGLES, countGreen, GL_UNSIGNED_INT, 0);

            // Draw Brown Mountains
            shader.setInt("materialType", 1);
            shader.setVec3("objectColor", glm::vec3(0.42f, 0.33f, 0.23f));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboBrown); glDrawElements(GL_TRIANGLES, countBrown, GL_UNSIGNED_INT, 0);

            // Draw Snow Peaks
            shader.setInt("materialType", 2);
            shader.setVec3("objectColor", glm::vec3(0.92f, 0.96f, 1.0f));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboSnow); glDrawElements(GL_TRIANGLES, countSnow, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void renderTrees(const Shader& shader) {
        shader.setInt("isParticle", 0);
        shader.setInt("useInstancing", 1);

        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.9f, 2.6f, 0.9f));
        shader.setMat4("model", model);
        shader.setInt("materialType", 5);
        shader.setVec3("objectColor", glm::vec3(0.34f, 0.19f, 0.08f));
        glBindVertexArray(trunkVAO);
        glDrawElementsInstanced(GL_TRIANGLES, trunkIndexCount, GL_UNSIGNED_INT, 0, (GLsizei)treePositions.size());

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 2.25f, 0.0f));
        model = glm::scale(model, glm::vec3(1.35f, 2.2f, 1.35f));
        shader.setMat4("model", model);
        shader.setInt("materialType", 6);
        shader.setVec3("objectColor", glm::vec3(0.08f, 0.28f, 0.10f));
        glBindVertexArray(leavesVAO);
        glDrawElementsInstanced(GL_TRIANGLES, leavesIndexCount, GL_UNSIGNED_INT, 0, (GLsizei)treePositions.size());

        shader.setInt("useInstancing", 0);
        glBindVertexArray(0);
    }

    void renderSkybox(Shader& shader,
                      const glm::mat4& view,
                      const glm::mat4& projection) {
        glDepthFunc(GL_LEQUAL);
        shader.use();
        shader.setMat4("view", glm::mat4(glm::mat3(view)));
        shader.setMat4("projection", projection);
        shader.setInt("skybox", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }

    unsigned int getSkyboxTexture() const {
        return skyboxTexture;
    }

    void renderSun(const Shader& shader, const glm::vec3& sunPosition, const glm::vec3& sunColor) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, sunPosition);
        model = glm::scale(model, glm::vec3(2.2f));
        shader.setMat4("model", model);
        shader.setInt("materialType", 3);
        shader.setInt("isParticle", 0);
        shader.setInt("useInstancing", 0);
        shader.setVec3("objectColor", sunColor);

        glDisable(GL_CULL_FACE);
        glBindVertexArray(sunVAO);
        glDrawElements(GL_TRIANGLES, sunIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glEnable(GL_CULL_FACE);
    }

    void renderShadows(const Shader& shader) {
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        shader.setInt("useInstancing", 0);
        glBindVertexArray(planeVAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboGreen); glDrawElements(GL_TRIANGLES, countGreen, GL_UNSIGNED_INT, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboBrown); glDrawElements(GL_TRIANGLES, countBrown, GL_UNSIGNED_INT, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboSnow); glDrawElements(GL_TRIANGLES, countSnow, GL_UNSIGNED_INT, 0);
    }

    void renderTreeShadows(const Shader& shader) {
        shader.setInt("useInstancing", 1);

        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.9f, 2.6f, 0.9f));
        shader.setMat4("model", model);
        glBindVertexArray(trunkVAO);
        glDrawElementsInstanced(GL_TRIANGLES, trunkIndexCount, GL_UNSIGNED_INT, 0, (GLsizei)treePositions.size());

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 2.25f, 0.0f));
        model = glm::scale(model, glm::vec3(1.35f, 2.2f, 1.35f));
        shader.setMat4("model", model);
        glBindVertexArray(leavesVAO);
        glDrawElementsInstanced(GL_TRIANGLES, leavesIndexCount, GL_UNSIGNED_INT, 0, (GLsizei)treePositions.size());

        shader.setInt("useInstancing", 0);
        glBindVertexArray(0);
    }

public:
        float terrainHeight(float x, float z) const {
            float center = std::sqrt(x * x + z * z);
            float ridgeA = std::exp(-std::pow((z + 8.0f) / 15.0f, 2.0f)) * (9.0f + 3.0f * std::sin(x * 0.16f));
            float ridgeB = std::exp(-std::pow((x - 12.0f) / 18.0f, 2.0f)) * (5.5f + 2.5f * std::cos(z * 0.13f));
            float rolling = std::sin(x * 0.18f) * std::cos(z * 0.14f) * 1.2f;
            float fineNoise = std::sin((x + z) * 0.65f) * std::cos((x - z) * 0.31f) * 0.45f;
            float meadowFade = glm::smoothstep(5.0f, 22.0f, center);
            return (ridgeA + ridgeB) * meadowFade + rolling + fineNoise - 1.2f;
        }

        glm::vec3 terrainNormal(float x, float z) const {
            float step = 0.35f;
            float hL = terrainHeight(x - step, z);
            float hR = terrainHeight(x + step, z);
            float hD = terrainHeight(x, z - step);
            float hU = terrainHeight(x, z + step);
            return glm::normalize(glm::vec3(hL - hR, 2.0f * step, hD - hU));
        }

        void initSunMesh() {
            std::vector<float> vertices;
            std::vector<unsigned int> indices;
            const unsigned int xSegments = 32;
            const unsigned int ySegments = 16;
            const float pi = 3.14159265359f;

            for (unsigned int y = 0; y <= ySegments; ++y) {
                for (unsigned int x = 0; x <= xSegments; ++x) {
                    float xSegment = (float)x / (float)xSegments;
                    float ySegment = (float)y / (float)ySegments;
                    float xPos = std::cos(xSegment * 2.0f * pi) * std::sin(ySegment * pi);
                    float yPos = std::cos(ySegment * pi);
                    float zPos = std::sin(xSegment * 2.0f * pi) * std::sin(ySegment * pi);

                    vertices.push_back(xPos);
                    vertices.push_back(yPos);
                    vertices.push_back(zPos);
                    vertices.push_back(xPos);
                    vertices.push_back(yPos);
                    vertices.push_back(zPos);
                    vertices.push_back(xSegment);
                    vertices.push_back(ySegment);
                }
            }

            for (unsigned int y = 0; y < ySegments; ++y) {
                for (unsigned int x = 0; x < xSegments; ++x) {
                    unsigned int a = y * (xSegments + 1) + x;
                    unsigned int b = (y + 1) * (xSegments + 1) + x;
                    indices.push_back(a);
                    indices.push_back(b);
                    indices.push_back(a + 1);
                    indices.push_back(a + 1);
                    indices.push_back(b);
                    indices.push_back(b + 1);
                }
            }
            sunIndexCount = (int)indices.size();

            glGenVertexArrays(1, &sunVAO);
            glGenBuffers(1, &sunVBO);
            glGenBuffers(1, &sunEBO);
            glBindVertexArray(sunVAO);
            glBindBuffer(GL_ARRAY_BUFFER, sunVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sunEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glBindVertexArray(0);
        }

        GLuint createPatternTexture(int kind) {
            const int texSize = 128;
            std::vector<unsigned char> data(texSize * texSize * 3);

            for (int y = 0; y < texSize; ++y) {
                for (int x = 0; x < texSize; ++x) {
                    float fx = (float)x / (float)texSize;
                    float fy = (float)y / (float)texSize;
                    float n = std::sin(fx * 52.0f + fy * 17.0f)
                            * std::cos(fx * 19.0f - fy * 47.0f);
                    float fine = std::sin((fx + fy) * 180.0f) * 0.5f + 0.5f;

                    glm::vec3 color;
                    if (kind == 0) {
                        color = glm::mix(glm::vec3(0.06f, 0.24f, 0.07f),
                                         glm::vec3(0.28f, 0.55f, 0.16f),
                                         0.45f + 0.35f * n + 0.20f * fine);
                    } else if (kind == 1) {
                        color = glm::mix(glm::vec3(0.22f, 0.21f, 0.19f),
                                         glm::vec3(0.55f, 0.46f, 0.35f),
                                         0.50f + 0.35f * n);
                    } else {
                        color = glm::mix(glm::vec3(0.70f, 0.82f, 0.90f),
                                         glm::vec3(1.0f, 1.0f, 0.97f),
                                         0.70f + 0.25f * fine + 0.10f * n);
                    }

                    int index = (y * texSize + x) * 3;
                    data[index + 0] = (unsigned char)(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
                    data[index + 1] = (unsigned char)(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
                    data[index + 2] = (unsigned char)(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
                }
            }

            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0,
                         GL_RGB, GL_UNSIGNED_BYTE, data.data());
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            return texture;
        }

        void initTerrainTextures() {
            grassTexture = createPatternTexture(0);
            rockTexture = createPatternTexture(1);
            snowTexture = createPatternTexture(2);
        }

        void initTreeMeshes() {
            const int segments = 14;
            const float pi = 3.14159265359f;
            std::vector<float> trunkVertices;
            std::vector<unsigned int> trunkIndices;
            std::vector<float> leavesVertices;
            std::vector<unsigned int> leavesIndices;

            for (int i = 0; i <= segments; ++i) {
                float a = (float)i / (float)segments * 2.0f * pi;
                float x = std::cos(a);
                float z = std::sin(a);
                trunkVertices.insert(trunkVertices.end(), {x * 0.18f, 0.0f, z * 0.18f, x, 0.0f, z, (float)i / segments, 0.0f});
                trunkVertices.insert(trunkVertices.end(), {x * 0.14f, 1.0f, z * 0.14f, x, 0.0f, z, (float)i / segments, 1.0f});
            }
            for (int i = 0; i < segments; ++i) {
                unsigned int a = i * 2;
                trunkIndices.insert(trunkIndices.end(), {a, a + 1, a + 2, a + 2, a + 1, a + 3});
            }

            leavesVertices.insert(leavesVertices.end(), {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f});
            for (int i = 0; i <= segments; ++i) {
                float a = (float)i / (float)segments * 2.0f * pi;
                float x = std::cos(a);
                float z = std::sin(a);
                glm::vec3 normal = glm::normalize(glm::vec3(x, 0.45f, z));
                leavesVertices.insert(leavesVertices.end(), {x * 0.85f, 0.0f, z * 0.85f, normal.x, normal.y, normal.z, (float)i / segments, 0.0f});
            }
            for (int i = 1; i <= segments; ++i) {
                leavesIndices.insert(leavesIndices.end(), {0u, (unsigned int)i, (unsigned int)(i + 1)});
            }

            trunkIndexCount = (int)trunkIndices.size();
            leavesIndexCount = (int)leavesIndices.size();

            glGenVertexArrays(1, &trunkVAO);
            glGenBuffers(1, &trunkVBO);
            glGenBuffers(1, &trunkEBO);
            glBindVertexArray(trunkVAO);
            glBindBuffer(GL_ARRAY_BUFFER, trunkVBO);
            glBufferData(GL_ARRAY_BUFFER, trunkVertices.size() * sizeof(float), trunkVertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trunkEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, trunkIndices.size() * sizeof(unsigned int), trunkIndices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);

            glGenVertexArrays(1, &leavesVAO);
            glGenBuffers(1, &leavesVBO);
            glGenBuffers(1, &leavesEBO);
            glBindVertexArray(leavesVAO);
            glBindBuffer(GL_ARRAY_BUFFER, leavesVBO);
            glBufferData(GL_ARRAY_BUFFER, leavesVertices.size() * sizeof(float), leavesVertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, leavesEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, leavesIndices.size() * sizeof(unsigned int), leavesIndices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glBindVertexArray(0);
        }

        void initTreeInstances() {
            glGenBuffers(1, &treeInstanceVBO);
            glBindBuffer(GL_ARRAY_BUFFER, treeInstanceVBO);
            glBufferData(GL_ARRAY_BUFFER,
                         treePositions.size() * sizeof(glm::vec3),
                         treePositions.data(), GL_STATIC_DRAW);

            glBindVertexArray(trunkVAO);
            glBindBuffer(GL_ARRAY_BUFFER, treeInstanceVBO);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            glBindVertexArray(leavesVAO);
            glBindBuffer(GL_ARRAY_BUFFER, treeInstanceVBO);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        void populateTrees() {
            for (int i = 0; i < 34; ++i) {
                float angle = (float)i * 1.713f;
                float radius = 12.0f + (float)(i % 7) * 3.3f;
                float x = std::cos(angle) * radius + (float)((i * 13) % 5 - 2);
                float z = std::sin(angle) * radius + (float)((i * 17) % 7 - 3);
                float y = terrainHeight(x, z);
                if (y < 4.0f) {
                    treePositions.push_back(glm::vec3(x, y, z));
                }
            }
        }

        glm::vec3 skyboxColor(int face, float u, float v) {
            glm::vec3 horizon(0.78f, 0.58f, 0.38f);
            glm::vec3 upper(0.12f, 0.22f, 0.46f);
            glm::vec3 night(0.015f, 0.020f, 0.060f);
            glm::vec3 cloud(0.90f, 0.86f, 0.78f);

            float vertical = glm::clamp(v, 0.0f, 1.0f);
            glm::vec3 color = glm::mix(horizon, upper, vertical);
            if (face == 2) {
                color = glm::mix(upper, glm::vec3(0.05f, 0.08f, 0.20f), 0.25f);
            } else if (face == 3) {
                color = glm::mix(night, horizon, 0.45f);
            }

            float cloudBand = std::sin(u * 20.0f + face * 1.7f)
                            * std::cos(v * 13.0f + face * 0.9f);
            if (cloudBand > 0.62f && face != 3) {
                color = glm::mix(color, cloud, 0.18f);
            }

            return color;
        }

        void initSkybox() {
            float skyboxVertices[] = {
                -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
                -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
            };

            glGenVertexArrays(1, &skyboxVAO);
            glGenBuffers(1, &skyboxVBO);
            glBindVertexArray(skyboxVAO);
            glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            const int texSize = 128;
            glGenTextures(1, &skyboxTexture);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            for (int face = 0; face < 6; ++face) {
                std::vector<unsigned char> data(texSize * texSize * 3);
                for (int y = 0; y < texSize; ++y) {
                    for (int x = 0; x < texSize; ++x) {
                        float u = (float)x / (float)(texSize - 1);
                        float v = (float)y / (float)(texSize - 1);
                        glm::vec3 color = skyboxColor(face, u, v);
                        int index = (y * texSize + x) * 3;
                        data[index + 0] = (unsigned char)(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
                        data[index + 1] = (unsigned char)(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
                        data[index + 2] = (unsigned char)(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
                    }
                }
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB,
                             texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glBindVertexArray(0);
        }

        std::vector<unsigned int> indicesSnow, indicesBrown, indicesGreen;
        std::vector<glm::vec3> treePositions;
        unsigned int planeVAO, planeVBO;
        unsigned int eboGreen, eboBrown, eboSnow;
        unsigned int sunVAO, sunVBO, sunEBO;
        unsigned int trunkVAO, trunkVBO, trunkEBO;
        unsigned int leavesVAO, leavesVBO, leavesEBO;
        unsigned int treeInstanceVBO;
        unsigned int grassTexture, rockTexture, snowTexture;
        unsigned int skyboxVAO, skyboxVBO, skyboxTexture;
        int countGreen, countBrown, countSnow, sunIndexCount;
        int trunkIndexCount, leavesIndexCount;
};
