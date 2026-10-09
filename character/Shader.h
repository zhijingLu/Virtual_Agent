#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);

    ~Shader();
    void use() const;
    // Set a mat4 uniform variable.
    // Commonly used for model, view, and projection matrices.
    void setMat4(
        const std::string& name,
        const glm::mat4& matrix
    ) const;

    // Set a vec3 uniform variable.
    // Commonly used for positions, directions, or colors.
    void setVec3(
        const std::string& name,
        const glm::vec3& value
    ) const;

    // Set a float uniform variable.
    void setFloat(
        const std::string& name,
        float value
    ) const;

    // Set an integer uniform variable.
    // Often used for texture units.
    void setInt(
        const std::string& name,
        int value
    ) const;
};