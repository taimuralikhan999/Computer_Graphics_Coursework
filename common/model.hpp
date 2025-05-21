#pragma once

#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Model {
public:
    Model(const char* path);
    void draw(unsigned int& shaderID);
    void deleteBuffers();

private:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    GLuint VAO, vertexBuffer, uvBuffer, normalBuffer;

    bool loadObj(const char* path,
        std::vector<glm::vec3>& outVertices,
        std::vector<glm::vec2>& outUVs,
        std::vector<glm::vec3>& outNormals);
    void setupBuffers();
};
