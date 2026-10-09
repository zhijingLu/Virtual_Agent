#include "Mesh.h"
#include <glad/glad.h>

Mesh::Mesh()
{
    float vertices[] =
    {
        // Front side (z = +0.5)
        -0.5f, -0.5f,  0.5f,   // 0: front bottom-left
         0.5f, -0.5f,  0.5f,   // 1: front bottom-right
         0.5f,  0.5f,  0.5f,   // 2: front top-right
        -0.5f,  0.5f,  0.5f,   // 3: front top-left

        // Back side (z = -0.5)
        -0.5f, -0.5f, -0.5f,   // 4: back bottom-left
         0.5f, -0.5f, -0.5f,   // 5: back bottom-right
         0.5f,  0.5f, -0.5f,   // 6: back top-right
        -0.5f,  0.5f, -0.5f    // 7: back top-left
    };
    // -------------------------------------------------
    // Cube indices
    //
    // 6 faces
    // 2 triangles per face
    // 3 indices per triangle
    //
    // Total = 6 * 2 * 3 = 36 indices
    // -------------------------------------------------

    unsigned int indices[] =
    {
        // Front
        0, 1, 2,
        2, 3, 0,

        // Back
        5, 4, 7,
        7, 6, 5,

        // Left
        4, 0, 3,
        3, 7, 4,

        // Right
        1, 5, 6,
        6, 2, 1,

        // Top
        3, 2, 6,
        6, 7, 3,

        // Bottom
        4, 5, 1,
        1, 0, 4
    };
    // Create one Vertex Array Object.
    glGenVertexArrays(1, &VAO);
    // Create one Vertex Buffer Object.
    glGenBuffers(1, &VBO);
    // Create one Element Buffer Object.
    glGenBuffers(1, &EBO);

    // Bind VAO first.
    glBindVertexArray(VAO);

    // Bind VBO.
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Copy vertex data from CPU memory to GPU memory.
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );
	// Bind EBO.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // Copy vertex data from CPU memory to GPU memory.
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
    );
    // Tell OpenGL how to interpret one vertex.
    glVertexAttribPointer(
        0,                  // attribute location
        3,                  // x, y, z
        GL_FLOAT,           // data type
        GL_FALSE,           // do not normalize
        3 * sizeof(float),  // size of one vertex
        (void*)0            // data starts at offset 0
    );
    // Enable vertex attribute location 0.
    glEnableVertexAttribArray(0);
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Mesh::draw()
{
    // Bind vertex configuration.
    glBindVertexArray(VAO);
    //  Draw cube using the index buffer.
    glDrawElements(
        GL_TRIANGLES,
        36,
        GL_UNSIGNED_INT,
        0
    );
}