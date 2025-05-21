#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "model.hpp"

Model::Model(const char* path) {
    bool success = loadObj(path, vertices, uvs, normals);
    if (!success) {
        std::cerr << "Failed to load model: " << path << std::endl;
    }
    setupBuffers();
}

void Model::draw(unsigned int& shaderID) {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
}

void Model::deleteBuffers() {
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &uvBuffer);
    glDeleteBuffers(1, &normalBuffer);
    glDeleteVertexArrays(1, &VAO);
}

bool Model::loadObj(const char* path,
    std::vector<glm::vec3>& outVertices,
    std::vector<glm::vec2>& outUVs,
    std::vector<glm::vec3>& outNormals) {
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> tempVertices;
    std::vector<glm::vec2> tempUVs;
    std::vector<glm::vec3> tempNormals;

    FILE* file = fopen(path, "r");
    if (!file) {
        std::cerr << "Cannot open file: " << path << std::endl;
        return false;
    }

    while (true) {
        char lineHeader[128];
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF) break;

        if (strcmp(lineHeader, "v") == 0) {
            glm::vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            tempVertices.push_back(vertex);
        }
        else if (strcmp(lineHeader, "vt") == 0) {
            glm::vec2 uv;
            fscanf(file, "%f %f\n", &uv.x, &uv.y);
            tempUVs.push_back(uv);
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            glm::vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            tempNormals.push_back(normal);
        }
        else if (strcmp(lineHeader, "f") == 0) {
            unsigned int vertexIndex[3], uvIndex[3] = { 0, 0, 0 }, normalIndex[3] = { 0, 0, 0 };
            int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
                &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                &vertexIndex[2], &uvIndex[2], &normalIndex[2]);

            if (matches != 9) {
                std::cerr << "Model format not supported (expected v/t/n format)" << std::endl;
                fclose(file);
                return false;
            }

            for (int i = 0; i < 3; ++i) {
                outVertices.push_back(tempVertices[vertexIndex[i] - 1]);

                glm::vec2 uv(0.0f);
                if (uvIndex[i] > 0 && uvIndex[i] <= tempUVs.size())
                    uv = tempUVs[uvIndex[i] - 1];
                outUVs.push_back(uv);

                glm::vec3 normal(0.0f);
                if (normalIndex[i] > 0 && normalIndex[i] <= tempNormals.size())
                    normal = tempNormals[normalIndex[i] - 1];
                outNormals.push_back(normal);
            }
        }
        else {
            // Skip the line
            char buffer[1000];
            fgets(buffer, 1000, file);
        }
    }

    fclose(file);
    return true;
}

void Model::setupBuffers() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Vertex buffer
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // UV buffer
    glGenBuffers(1, &uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), uvs.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Normal buffer
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
}
