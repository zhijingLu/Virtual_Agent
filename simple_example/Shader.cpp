#include "Shader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexCode;
    std::string fragmentCode;

    // Read vertex shader file.
    std::ifstream vertexFile(vertexPath);

    // Read fragment shader file.
    std::ifstream fragmentFile(fragmentPath);

    if (!vertexFile.is_open())
    {
        std::cerr << "Failed to open vertex shader file."
            << std::endl;
    }

    if (!fragmentFile.is_open())
    {
        std::cerr << "Failed to open fragment shader file."
            << std::endl;
    }

    std::stringstream vertexStream;
    std::stringstream fragmentStream;
    // Read all contents from the files.
    vertexStream << vertexFile.rdbuf();
    fragmentStream << fragmentFile.rdbuf();
    // Convert stream contents to std::string.
    vertexCode = vertexStream.str();
    fragmentCode = fragmentStream.str();
    // OpenGL expects C-style strings.
    const char* vertexSource = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();

    // -------------------------
    // Compile Vertex Shader
    // -------------------------

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(
        vertexShader,
        1,
        &vertexSource,
        nullptr
    );

    glCompileShader(vertexShader);

    // -------------------------
    // Compile Fragment Shader
    // -------------------------

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(
        fragmentShader,
        1,
        &fragmentSource,
        nullptr
    );

    glCompileShader(fragmentShader);

    // -------------------------
    // Link Shader Program
    // -------------------------

    ID = glCreateProgram();

    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);

    glLinkProgram(ID);

    // Shader objects are no longer needed.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(ID);
}

void Shader::use() const
{
    glUseProgram(ID);
}

void Shader::setMat4(
    const std::string& name,
    const glm::mat4& matrix
) const
{
    // Find the location of the uniform variable
    // inside the shader program.
    int location = glGetUniformLocation(
        ID,
        name.c_str()
    );

    // Send a 4x4 matrix to the shader.
    glUniformMatrix4fv(
        location,               // uniform location
        1,                      // number of matrices
        GL_FALSE,               // do not transpose
        glm::value_ptr(matrix)  // pointer to matrix data
    );
}


void Shader::setVec3(
    const std::string& name,
    const glm::vec3& value
) const
{
    // Find uniform location.
    int location = glGetUniformLocation(
        ID,
        name.c_str()
    );

    // Send a vec3 to the shader.
    glUniform3fv(
        location,
        1,
        glm::value_ptr(value)
    );
}


void Shader::setFloat(
    const std::string& name,
    float value
) const
{
    // Find uniform location.
    int location = glGetUniformLocation(
        ID,
        name.c_str()
    );

    // Send one float value to the shader.
    glUniform1f(
        location,
        value
    );
}


void Shader::setInt(
    const std::string& name,
    int value
) const
{
    // Find uniform location.
    int location = glGetUniformLocation(
        ID,
        name.c_str()
    );

    // Send one integer value to the shader.
    glUniform1i(
        location,
        value
    );
}