#pragma once

#include <vector>

#include <glm/glm.hpp>


// Maximum number of bones that can affect one vertex.
const int MAX_BONE_INFLUENCE = 4;


// Data stored for each vertex.
struct Vertex
{
    // Vertex position in model space.
    glm::vec3 position;

    // Surface normal.
    glm::vec3 normal;

    // Texture coordinates.
    glm::vec2 texCoords;

    // IDs of bones affecting this vertex.
    //
    // -1 means the slot is unused.
    glm::ivec4 boneIDs = glm::ivec4(-1);

    // Influence weight of each corresponding bone.
    glm::vec4 weights =glm::vec4(0.0f);
};


class Mesh
{
public:

    // Create a mesh from vertex/index data
    // and an OpenGL texture ID.
    Mesh(
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        unsigned int textureID
    );


    // Release GPU resources owned by this Mesh.
    ~Mesh();


    // -------------------------------------------------
    // Copying is disabled
    // -------------------------------------------------

    // GPU resource IDs should not be copied,
    // otherwise multiple Mesh objects could think
    // they own the same VAO/VBO/EBO/texture.
    Mesh(const Mesh&) = delete;

    Mesh& operator=(
        const Mesh&
        ) = delete;


    // -------------------------------------------------
    // Moving is allowed
    // -------------------------------------------------

    // Transfer ownership from another Mesh.
    Mesh(
        Mesh&& other
    ) noexcept;


    // Transfer ownership into an existing Mesh.
    Mesh& operator=(
        Mesh&& other
        ) noexcept;


    // Draw this mesh.
    void draw();


private:

    // Vertex Array Object.
    unsigned int VAO = 0;

    // Vertex Buffer Object.
    unsigned int VBO = 0;

    // Element / Index Buffer Object.
    unsigned int EBO = 0;

    // OpenGL texture ID used by this mesh.
    unsigned int textureID = 0;

    // Number of indices in the EBO.
    unsigned int indexCount = 0;


    // Create VAO/VBO/EBO and configure
    // all vertex attributes.
    void setupMesh(
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices
    );
};