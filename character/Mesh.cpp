#include "Mesh.h"

#include <glad/glad.h>
#include <cstddef>

Mesh::Mesh(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    unsigned int textureID
)
{
    // Save the number of indices.
    //
    // We need this later in glDrawElements().
    indexCount =
        static_cast<unsigned int>(indices.size());
    this->textureID = textureID;
    // Create the OpenGL buffers and upload data.
    setupMesh(vertices, indices);
}
// Move constructor  = b is not esist, create a new Mesh object by transferring ownership of OpenGL resources from another Mesh object.
Mesh::Mesh(Mesh&& other) noexcept
{
    // Transfer ownership of OpenGL resources.
    VAO = other.VAO;
    VBO = other.VBO;
    EBO = other.EBO;
    textureID = other.textureID;
    indexCount = other.indexCount;

    // The old object no longer owns these resources.
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
	other.textureID = 0;
    other.indexCount = 0;
}
// Move assignment operator, =b is esist, transfer ownership of OpenGL resources from another Mesh object to this Mesh object, and delete any existing resources owned by this Mesh object.
Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other)
    {
        // Delete resources currently owned by this object.
        if (VAO != 0)
            glDeleteVertexArrays(1, &VAO);

        if (VBO != 0)
            glDeleteBuffers(1, &VBO);

        if (EBO != 0)
            glDeleteBuffers(1, &EBO);

        // Take ownership from the other object.
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        textureID = other.textureID;
        indexCount = other.indexCount;

        // Prevent the moved-from object from deleting them.
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
		other.textureID = 0;
        other.indexCount = 0;
    }

    return *this;
}
Mesh::~Mesh()
{
    if (VAO != 0)
        glDeleteVertexArrays(1, &VAO);

    if (VBO != 0)
        glDeleteBuffers(1, &VBO);

    if (EBO != 0)
        glDeleteBuffers(1, &EBO);
	if (textureID != 0)
		glDeleteTextures(1, &textureID);
}


void Mesh::setupMesh(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices
)
{
    // -------------------------------------------------
    // 1. Generate OpenGL objects
    // -------------------------------------------------

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);


    // -------------------------------------------------
    // 2. Bind VAO
    // -------------------------------------------------

    glBindVertexArray(VAO);


    // -------------------------------------------------
    // 3. Upload vertex data
    // -------------------------------------------------

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,

        // Total byte size of the vertex array.
        vertices.size() * sizeof(Vertex),

        // Pointer to the first vertex.
        vertices.data(),

        GL_STATIC_DRAW
    );


    // -------------------------------------------------
    // 4. Upload index data
    // -------------------------------------------------

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBufferData(  
        GL_ELEMENT_ARRAY_BUFFER, //target,

        // Total byte size of the index array.
        indices.size() * sizeof(unsigned int), //  size,

        // Pointer to the first index.
        indices.data(),//  data,

        GL_STATIC_DRAW //usage
    );


    // -------------------------------------------------
    // Vertex attribute 0: position
    // -------------------------------------------------

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, position)
    );

    glEnableVertexAttribArray(0);
    // -------------------------------------------------
    // Vertex attribute 1: normal
    // -------------------------------------------------

    glVertexAttribPointer(
        1, //index
        3, //size
        GL_FLOAT,  //type
        GL_FALSE,  //normalized,
        sizeof(Vertex), //stride
        (void*)offsetof(Vertex, normal)  //pointer
    );

    glEnableVertexAttribArray(1);


    // -------------------------------------------------
    // Vertex attribute 2: texture coordinates
    // -------------------------------------------------

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, texCoords)
    );

    glEnableVertexAttribArray(2);

    // =================================================
    // Vertex attribute 3: Bone IDs
    // =================================================

    // Bone IDs are integers,
    // so we use glVertexAttribIPointer.
    //
    // The "I" means integer attribute.
    glVertexAttribIPointer(
        3,                                  // location
        4,                                  // 4 bone IDs
        GL_INT,                             // integer data
        sizeof(Vertex),                     // stride
        (void*)offsetof(
            Vertex,
            boneIDs
        )
    );


    glEnableVertexAttribArray(3);

    // =================================================
    // Vertex attribute 4: Bone weights
    // =================================================

    glVertexAttribPointer(
        4,                                  // location
        4,                                  // 4 weights
        GL_FLOAT,                           // float data
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(
            Vertex,
            weights
        )
    );
    glEnableVertexAttribArray( 4);
    // -------------------------------------------------
	// 6. Unbind VAO, so that we don't accidentally modify it later.
    // -------------------------------------------------

    glBindVertexArray(0);  
}


void Mesh::draw()
{
    // Use texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the character texture.
    glBindTexture(
        GL_TEXTURE_2D,
        textureID
    );
    // Activate this mesh's vertex configuration.
    glBindVertexArray(VAO);

    // Draw using the index buffer.
    glDrawElements(
        GL_TRIANGLES,
        indexCount,
        GL_UNSIGNED_INT,
        nullptr
    );

    glBindVertexArray(0);
}