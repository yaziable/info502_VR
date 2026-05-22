#define GLAD_GLES_IMPLEMENTATION 0
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <filesystem>

#include "Shader.h"
#include "Camera.h"
#include "ShadowCreature.h"
#include "AILightController.h"
#include "Scene.h"
#include "LoadedModel.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void dragParticle(GLFWwindow* window, std::vector<ShadowCreature>& creatures);
void readShadowMapDepth(GLuint shadowMap, int width, int height, std::vector<float>& depthBuffer);
bool isCreatureInShadowMap(const glm::vec3& position,
                           const glm::mat4& lightSpaceMatrix,
                           const std::vector<float>& shadowDepth,
                           int width,
                           int height);
bool isCreatureInShade(const ShadowCreature& creature,
                       const Scene& scene,
                       const std::vector<ShadowCreature>& creatures,
                       const glm::mat4& lightSpaceMatrix,
                       const std::vector<float>& shadowDepth,
                       int shadowMapSize,
                       glm::vec3 lightPos,
                       float sunIntensity);

Camera camera(glm::vec3(0.0f, 6.0f, 28.0f));
ShadowCreature* selectedCreature = nullptr;
float dragDistance = 5.0f;
float deltaTime = 0.0f;
float lastX = 800.0f / 2.0f;
float lastY = 600.0f / 2.0f;
bool firstMouse = true;

int main(int argc, char** argv) {
    if (argc > 0) {
        std::filesystem::path exePath = std::filesystem::absolute(argv[0]);
        std::filesystem::current_path(exePath.parent_path());
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720,
        "Shadow Creatures - 3D Graphics Project", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    Scene scene;
    AILightController aiLights;
    std::vector<ShadowCreature> creatures;
    const int shadowMapSize = 2048;
    std::vector<float> shadowDepth(shadowMapSize * shadowMapSize);

    // Spawn créatures initiales
    for (int i = 0; i < 15; i++) {
        float cx = (float)(rand()%60-30);
        float cz = (float)(rand()%60-30);
        float cy = scene.terrainHeight(cx, cz) + 0.3f;
        creatures.emplace_back(glm::vec3(cx, cy, cz));
    }

    // Shadow mapping FBO
    GLuint shadowMapFBO, shadowMap;
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    Shader creatureShader("shaders/shadow_creature.vert",
                          "shaders/shadow_creature.frag");
    Shader shadowShader("shaders/shadow_map.vert",
                        "shaders/shadow_map.frag");
    Shader skyboxShader("shaders/skybox.vert",
                        "shaders/skybox.frag");
    LoadedModel bunnyModel("models/bunny_small.obj");
    LoadedModel sphereModel("models/sphere_smooth.obj");

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        camera.update(deltaTime);
        aiLights.update(deltaTime);

        // Shadow pass
        glViewport(0, 0, 2048, 2048);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix",
                             aiLights.getLightSpaceMatrix());
        scene.renderShadows(shadowShader);
        scene.renderTreeShadows(shadowShader);
        bunnyModel.renderShadow(shadowShader,
                                glm::vec3(-4.0f, scene.terrainHeight(-4.0f, 16.0f) + 0.4f, 16.0f),
                                glm::vec3(1.5f));
        sphereModel.renderShadow(shadowShader,
                                 glm::vec3(4.0f, scene.terrainHeight(4.0f, 16.0f) + 1.2f, 16.0f),
                                 glm::vec3(1.8f));
        for (auto& creature : creatures) {
            creature.renderShadow(shadowShader);
        }

        // Read back the GPU shadow map depth texture for CPU shade tests.
        readShadowMapDepth(shadowMap, shadowMapSize, shadowMapSize, shadowDepth);
        const glm::mat4 lightSpaceMatrix = aiLights.getLightSpaceMatrix();

        // Main pass
        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        float sunIntensity = aiLights.getIntensity();
        glm::vec3 dawnSky(0.98f, 0.48f, 0.28f);
        glm::vec3 daySky(0.47f, 0.73f, 0.96f);
        glm::vec3 nightSky(0.02f, 0.03f, 0.08f);
        glm::vec3 skyColor = glm::mix(dawnSky, daySky,
            glm::smoothstep(0.32f, 0.85f, sunIntensity));
        skyColor = glm::mix(nightSky, skyColor,
            glm::smoothstep(0.08f, 0.35f, sunIntensity));
        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        scene.renderSkybox(skyboxShader,
                           camera.getViewMatrix(),
                           camera.getProjectionMatrix());

        creatureShader.use();
        creatureShader.setMat4("view", camera.getViewMatrix());
        creatureShader.setMat4("projection", camera.getProjectionMatrix());
        creatureShader.setMat4("lightSpaceMatrix",
                               aiLights.getLightSpaceMatrix());
        creatureShader.setInt("shadowMap", 0);
        creatureShader.setVec3("viewPos", camera.getPosition());
        creatureShader.setVec3("lightPos", aiLights.getPosition());
        creatureShader.setVec3("lightColor", aiLights.getLightColor());
        creatureShader.setFloat("sunIntensity", sunIntensity);
        creatureShader.setInt("skybox", 4);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene.getSkyboxTexture());
        glActiveTexture(GL_TEXTURE0);

        // Terrain et soleil en premier pour remplir le depth buffer
        scene.render(creatureShader);
        scene.renderTrees(creatureShader);
        bunnyModel.render(creatureShader,
                          glm::vec3(-4.0f, scene.terrainHeight(-4.0f, 16.0f) + 0.4f, 16.0f),
                          glm::vec3(1.5f),
                          glm::vec3(0.86f, 0.62f, 0.36f),
                          4);
        sphereModel.render(creatureShader,
                           glm::vec3(4.0f, scene.terrainHeight(4.0f, 16.0f) + 1.2f, 16.0f),
                           glm::vec3(1.8f),
                           glm::vec3(0.38f, 0.62f, 0.92f),
                           4);
        scene.renderSun(creatureShader,
                        aiLights.getPosition(),
                        aiLights.getLightColor());

        // Update + render créatures
        for (auto it = creatures.begin(); it != creatures.end(); ) {
            //it->update(deltaTime, shadowMap, aiLights.getPosition());
            bool inShade = isCreatureInShade(
                *it, scene, creatures,
                lightSpaceMatrix, shadowDepth, shadowMapSize,
                aiLights.getPosition(), sunIntensity);
            it->update(deltaTime, shadowMap,
                       aiLights.getPosition(), sunIntensity,
                       aiLights.getLightColor(), inShade); // Rajout de aiLights.getLightColor()

            // Blob non-morphé trop petit → supprimé.
            // Une créature morphée reste comme polygone permanent dans la scène.
            bool dead = (!it->isMorphed && it->size < 0.04f);
            if (dead) {
                it = creatures.erase(it);
            } else {
                it->render(creatureShader);
                ++it;
            }
        }

        // Spawn continuel — max 20 créatures non-morphées actives
        int activeCount = 0;
        for (auto& c : creatures) if (!c.isMorphed) activeCount++;
        if (activeCount < 20 && rand() % 60 < 1) {
            float sx = (float)(rand()%60-30);
            float sz = (float)(rand()%60-30);
            float sy = scene.terrainHeight(sx, sz) + 0.3f;
            creatures.emplace_back(glm::vec3(sx, sy, sz));
        }

        dragParticle(window, creatures);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Shadow Creatures Control");
        ImGui::Text("Sun cycle: %.0f%% daylight", sunIntensity * 100.0f);
        ImGui::Text("Creatures: %d", (int)creatures.size());
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::Text("  Clic droit + souris : orienter camera");
        ImGui::Text("  Clic milieu + souris : pan camera");
        ImGui::Text("  Clic gauche : attraper creature");
        ImGui::Text("  W/A/S/D : deplacer camera");
        ImGui::Text("  Scroll : zoom");
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

bool isTerrainBlockingSun(const glm::vec3& position,
                          const Scene& scene,
                          glm::vec3 lightPos) {
    glm::vec3 toLight = glm::normalize(lightPos - position);
    if (toLight.y <= 0.05f) return false;

    glm::vec3 start = position + glm::vec3(0.0f, 0.5f, 0.0f);
    for (float distance = 1.0f; distance < 36.0f; distance += 0.8f) {
        glm::vec3 sample = start + toLight * distance;
        if (scene.terrainHeight(sample.x, sample.z) > sample.y + 0.2f) {
            return true;
        }
    }

    return false;
}

bool isPolygonBlockingSun(const ShadowCreature& creature,
                          const std::vector<ShadowCreature>& creatures,
                          glm::vec3 lightPos,
                          float sunIntensity) {
    for (const auto& polygon : creatures) {
        if (&polygon == &creature || !polygon.isMorphed) continue;

        glm::vec2 shadowDir(
            polygon.position.x - lightPos.x,
            polygon.position.z - lightPos.z);
        float dirLength = glm::length(shadowDir);
        if (dirLength < 0.001f) continue;
        shadowDir /= dirLength;

        glm::vec2 toCreature(
            creature.position.x - polygon.position.x,
            creature.position.z - polygon.position.z);
        float alongShadow = glm::dot(toCreature, shadowDir);
        if (alongShadow <= 0.0f) continue;

        float shadowLength = 7.0f + (1.0f - sunIntensity) * 28.0f;
        float shadowWidth = glm::max(1.8f, polygon.size * 1.8f);
        glm::vec2 closestPoint = shadowDir * alongShadow;
        float distanceFromShadow = glm::length(toCreature - closestPoint);

        if (alongShadow < shadowLength && distanceFromShadow < shadowWidth) {
            return true;
        }
    }

    return false;
}

void readShadowMapDepth(GLuint shadowMap, int width, int height, std::vector<float>& depthBuffer) {
    if ((int)depthBuffer.size() != width * height) {
        depthBuffer.assign(width * height, 1.0f);
    }
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depthBuffer.data());
}

bool isCreatureInShadowMap(const glm::vec3& position,
                           const glm::mat4& lightSpaceMatrix,
                           const std::vector<float>& shadowDepth,
                           int width,
                           int height) {
    glm::vec4 worldPos = glm::vec4(position, 1.0f);
    glm::vec4 lightSpacePos = lightSpaceMatrix * worldPos;
    if (std::abs(lightSpacePos.w) < 1e-6f) return false;

    glm::vec3 projCoords = glm::vec3(lightSpacePos) / lightSpacePos.w;
    projCoords = projCoords * 0.5f + 0.5f;
    if (projCoords.x < 0.0f || projCoords.x > 1.0f
     || projCoords.y < 0.0f || projCoords.y > 1.0f
     || projCoords.z > 1.0f) {
        return false;
    }

    int px = std::clamp((int)(projCoords.x * width), 0, width - 1);
    int py = std::clamp((int)(projCoords.y * height), 0, height - 1);
    float closestDepth = shadowDepth[py * width + px];
    float bias = 0.005f;
    return projCoords.z - bias > closestDepth;
}

bool isCreatureInShade(const ShadowCreature& creature,
                       const Scene& scene,
                       const std::vector<ShadowCreature>& creatures,
                       const glm::mat4& lightSpaceMatrix,
                       const std::vector<float>& shadowDepth,
                       int shadowMapSize,
                       glm::vec3 lightPos,
                       float sunIntensity) {
    if (creature.isMorphed) return false;
    if (sunIntensity < 0.18f) return true;

    bool inShadowMap = isCreatureInShadowMap(
        creature.position, lightSpaceMatrix,
        shadowDepth, shadowMapSize, shadowMapSize);

    return inShadowMap
        || isTerrainBlockingSun(creature.position, scene, lightPos)
        || isPolygonBlockingSun(creature, creatures, lightPos, sunIntensity);
}

void dragParticle(GLFWwindow* window,
                  std::vector<ShadowCreature>& creatures) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        // Rayon caméra → curseur en coordonnées monde
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float ndcX =  (2.0f * (float)xpos) / (float)width  - 1.0f;
        float ndcY = -(2.0f * (float)ypos) / (float)height + 1.0f;

        glm::mat4 invVP = glm::inverse(
            camera.getProjectionMatrix() * camera.getViewMatrix());
        glm::vec4 nearPt = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        nearPt /= nearPt.w;
        glm::vec4 farPt  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
        farPt  /= farPt.w;
        glm::vec3 rayDir = glm::normalize(glm::vec3(farPt) - glm::vec3(nearPt));

        if (!selectedCreature) {
            float minDist = 5.0f;
            for (auto& c : creatures) {
                glm::vec3 toCreature = c.position - camera.Position;
                float distToRay = glm::length(glm::cross(rayDir, toCreature));
                if (distToRay < minDist && glm::dot(rayDir, toCreature) > 0) {
                    minDist = distToRay;
                    selectedCreature = &c;
                }
            }
        }

        // Déplacer la créature : intersection du rayon avec le plan horizontal
        if (selectedCreature && std::abs(rayDir.y) > 1e-4f) {
            float t = (selectedCreature->position.y - camera.Position.y) / rayDir.y;
            if (t > 0.0f) {
                glm::vec3 hit = camera.Position + rayDir * t;
                selectedCreature->position.x = hit.x;
                selectedCreature->position.z = hit.z;
            }
        }
    } else {
        selectedCreature = nullptr;
    }
}

void framebuffer_size_callback(GLFWwindow* window,
                                int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos; lastY = ypos;
        firstMouse = false; return;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;

    if (ImGui::GetIO().WantCaptureMouse) return;

    bool dragging = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (!dragging && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        camera.processMouseMovement(xoffset, yoffset);
    } else if (!dragging && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        camera.processMousePan(xoffset, yoffset);
    }
}

void mouse_button_callback(GLFWwindow* window,
                            int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT
     || button == GLFW_MOUSE_BUTTON_MIDDLE) {
        firstMouse = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (action == GLFW_RELEASE) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void scroll_callback(GLFWwindow* window,
                     double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    camera.processMouseScroll((float)yoffset);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);
}
