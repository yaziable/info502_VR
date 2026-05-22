#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Shader;
struct aiMesh;
struct aiNode;
struct aiScene;

class LoadedModel {
public:
    explicit LoadedModel(const std::string& path);

    bool isLoaded() const;
    void render(const Shader& shader,
                const glm::vec3& position,
                const glm::vec3& scale,
                const glm::vec3& color,
                int materialType = -1) const;
    void renderShadow(const Shader& shader,
                      const glm::vec3& position,
                      const glm::vec3& scale) const;

private:
    struct Mesh {
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
        GLsizei indexCount = 0;
    };

    std::vector<Mesh> meshes;
    bool loaded = false;

    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh);
};
