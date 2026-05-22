#include "LoadedModel.h"
#include "Shader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

LoadedModel::LoadedModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality
    );

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        std::cout << "MODEL LOAD ERROR: " << path << " - "
                  << importer.GetErrorString() << std::endl;
        return;
    }

    processNode(scene->mRootNode, scene);
    loaded = !meshes.empty();
    std::cout << "Loaded model: " << path << " ("
              << meshes.size() << " mesh(es))" << std::endl;
}

bool LoadedModel::isLoaded() const {
    return loaded;
}

void LoadedModel::render(const Shader& shader,
                         const glm::vec3& position,
                         const glm::vec3& scale,
                         const glm::vec3& color,
                         int materialType) const {
    if (!loaded) return;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    shader.setMat4("model", model);
    shader.setInt("isParticle", 0);
    shader.setInt("useInstancing", 0);
    shader.setInt("materialType", materialType);
    shader.setVec3("objectColor", color);

    for (const Mesh& mesh : meshes) {
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void LoadedModel::renderShadow(const Shader& shader,
                               const glm::vec3& position,
                               const glm::vec3& scale) const {
    if (!loaded) return;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    shader.setMat4("model", model);
    shader.setInt("useInstancing", 0);

    for (const Mesh& mesh : meshes) {
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void LoadedModel::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        processMesh(scene->mMeshes[node->mMeshes[i]]);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene);
    }
}

void LoadedModel::processMesh(aiMesh* mesh) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(mesh->mNumVertices * 8);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);

        if (mesh->HasNormals()) {
            vertices.push_back(mesh->mNormals[i].x);
            vertices.push_back(mesh->mNormals[i].y);
            vertices.push_back(mesh->mNormals[i].z);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }

        if (mesh->HasTextureCoords(0)) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh result;
    result.indexCount = (GLsizei)indices.size();

    glGenVertexArrays(1, &result.VAO);
    glGenBuffers(1, &result.VBO);
    glGenBuffers(1, &result.EBO);

    glBindVertexArray(result.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, result.VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    meshes.push_back(result);
}
