#include "Model.h"
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

// IMPORTANT:
// STB_IMAGE_IMPLEMENTATION must appear
// in exactly ONE .cpp file in the whole project.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
Model::Model(const std::string& path)
{
    // Load the model as soon as the Model object is created.
    loadModel(path);
}


void Model::draw(Shader& shader)
{
    // Draw every mesh contained in this model.
    //
    // For now Shader is not used directly inside Mesh,
    // but we keep it here because later materials/textures
    // will need access to the active shader.
    for (Mesh& mesh : meshes)
    {
        mesh.draw();
    }
}



void Model::loadModel(const std::string& path)
{
    // -------------------------------------------------
    // 1. Create Assimp importer
    // -------------------------------------------------

    Assimp::Importer importer;


    // -------------------------------------------------
    // 2. Read model file
    // -------------------------------------------------

    const aiScene* scene = importer.ReadFile(
        path,

        // Convert polygons into triangles.
        aiProcess_Triangulate |

        // Flip UV coordinates.
        // Useful for OpenGL texture conventions.
        aiProcess_FlipUVs |

        // Generate normals if the model does not contain them.
        aiProcess_GenNormals
    );


    // -------------------------------------------------
    // 3. Validate imported scene
    // -------------------------------------------------

    if (
        !scene ||
        scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode
        )
    {
        std::cerr
            << "ASSIMP ERROR: "
            << importer.GetErrorString()
            << std::endl;

        return;
    }


    // -------------------------------------------------
    // 4. Store model directory
    // -------------------------------------------------

    // Example:
    //
    // path =
    // assets/character/character.glb
    //
    // directory =
    // assets/character
    //
    std::size_t slashPosition =
        path.find_last_of("/\\");

    if (slashPosition != std::string::npos)
    {
        directory =
            path.substr(0, slashPosition);
    }
    else
    {
        directory = ".";
    }


    // -------------------------------------------------
    // 5. Process complete scene hierarchy
    // -------------------------------------------------
    //std::cout << "Meshes: " << scene->mNumMeshes << std::endl;
    processNode(
        scene->mRootNode,
        scene
    );
}


void Model::processNode(
    aiNode* node,
    const aiScene* scene
)
{
    // -------------------------------------------------
    // 1. Process all meshes referenced by this node
    // -------------------------------------------------
    //std::cout << "Node meshes: " << node->mNumMeshes << std::endl;
    for (
        unsigned int i = 0;
        i < node->mNumMeshes;
        ++i
        )
    {
        // node->mMeshes contains indices,
        // not actual mesh objects.
        aiMesh* mesh =
            scene->mMeshes[
                node->mMeshes[i]
            ];

        // Convert Assimp mesh into our own Mesh class.
        meshes.push_back(
            processMesh(mesh, scene)
        );
    }


    // -------------------------------------------------
    // 2. Recursively process child nodes
    // -------------------------------------------------

    for (
        unsigned int i = 0;
        i < node->mNumChildren;
        ++i
        )
    {
        processNode(
            node->mChildren[i],
            scene
        );
    }
}


Mesh Model::processMesh(
    aiMesh* mesh,
    const aiScene* scene
)
{
    // Our own vertex and index arrays.
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    //std::cout
    //    << "Vertices: " << mesh->mNumVertices
    //    << ", Faces: " << mesh->mNumFaces
    //    << std::endl;
    // -------------------------------------------------
    // 1. Read vertices
    // -------------------------------------------------

    for (
        unsigned int i = 0;
        i < mesh->mNumVertices;
        ++i
        )
    {
        Vertex vertex{};

        // ---------------------------------------------
        // Position
        // ---------------------------------------------
        vertex.position.x =
            mesh->mVertices[i].x;

        vertex.position.y =
            mesh->mVertices[i].y;

        vertex.position.z =
            mesh->mVertices[i].z;

        
        // ---------------------------------------------
        // Normal
        // ---------------------------------------------
        if (mesh->HasNormals())
        {
            vertex.normal.x =
                mesh->mNormals[i].x;

            vertex.normal.y =
                mesh->mNormals[i].y;

            vertex.normal.z =
                mesh->mNormals[i].z;
        }
        else
        {
            // Fallback in case the mesh has no normal.
            vertex.normal =
                glm::vec3(0.0f);
        }
        // ---------------------------------------------
        // Texture coordinates
        // ---------------------------------------------

        // Assimp supports multiple UV sets.
        // We use the first one: mTextureCoords[0].
        if (mesh->mTextureCoords[0])
        {
            vertex.texCoords.x =
                mesh->mTextureCoords[0][i].x;

            vertex.texCoords.y =
                mesh->mTextureCoords[0][i].y;
        }
        else
        {
            // If the model has no UV coordinates,
            // use (0, 0) as fallback.
            vertex.texCoords =
                glm::vec2(0.0f);
        }
        vertices.push_back(vertex);
    }


    // -------------------------------------------------
    // 2. Read triangle indices
    // -------------------------------------------------

    for (
        unsigned int i = 0;
        i < mesh->mNumFaces;
        ++i
        )
    {
        aiFace face =
            mesh->mFaces[i];

        // Because aiProcess_Triangulate was enabled,
        // each face should normally contain 3 indices.
        for (
            unsigned int j = 0;
            j < face.mNumIndices;
            ++j
            )
        {
            indices.push_back(
                face.mIndices[j]
            );
        }
    }
    // -------------------------------------------------
    // Read bone IDs + weights
    // -------------------------------------------------

    if (mesh->HasBones())
    {
        extractBoneWeightForVertices(
            vertices,
            mesh
        );
    }

    // -------------------------------------------------
    // Load material texture
    // -------------------------------------------------

    unsigned int textureID = 0;

    // Check whether this mesh has a material.
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material =
            scene->mMaterials[
                mesh->mMaterialIndex
            ];
        // glTF 2.0 normally uses a base color texture.
        textureID =
            loadMaterialTexture(
                material,
                aiTextureType_BASE_COLOR,
                scene
            );
		// if the base color texture is not found, try the diffuse texture.
        if (textureID == 0)
        {
            textureID =
                loadMaterialTexture(
                    material,
                    aiTextureType_DIFFUSE,
                    scene
                );
        }
    }

    // -------------------------------------------------
    // 3. Create our Mesh object
    // -------------------------------------------------

    return Mesh(
        vertices,
        indices,
        textureID
    );
}
// Load one texture from an Assimp material.
// determine whether it is embedded or an external file, and decode it into pixel data.
unsigned int Model::loadMaterialTexture(
    aiMaterial* material,
    aiTextureType type,
    const aiScene* scene
)
{
    // Check whether this material contains any texture of the requested type.
    //
    // Example:
    // type = aiTextureType_BASE_COLOR
    // or
    // type = aiTextureType_DIFFUSE


    // No texture of this type.
    if (material->GetTextureCount(type) == 0)
    {
        return 0;
    }

    // Assimp stores the texture reference/path inside an aiString.
    aiString texturePath;

    // Get the first texture of this type.
    // type:
    //     Which kind of texture we want.
    // 0:
    //     The first texture of this type.
    // &texturePath:
    //     Assimp writes the texture path/reference here.
    if (
        material->GetTexture(
            type,
            0,
            &texturePath
        ) != AI_SUCCESS
        )
    {
        return 0;
    }


    // -----------------------------------------
    // Case 1:
    // Embedded texture
    // Try to find the texture inside the imported scene.
    //
    // For .glb files, textures are often embedded directly inside the model file.
    //
    // If the texture is embedded, embeddedTexture will point to it.
    //
    // If it is not embedded, embeddedTexture will be nullptr.
    // Common for .glb files.
    // stb_image: Decode PNG/JPG images
    // -----------------------------------------

    const aiTexture* embeddedTexture =
        scene->GetEmbeddedTexture(
            texturePath.C_Str()
        );
    // Case 1:
    // The texture is embedded inside the model file.
    // This is common for .glb files.
    if (embeddedTexture)
    {
        int width = 0;
        int height = 0;
        int channels = 0;


        // mHeight == 0 means compressed image data,
        // such as PNG or JPEG stored inside the model.
        if (embeddedTexture->mHeight == 0)
        {
            // Decode the compressed image data
            // directly from memory.
            // pcData:
            //     Pointer to the compressed image bytes.
            // mWidth:
            //     For compressed textures, this represents
            //     the total size of the compressed data
            //     in bytes.
            unsigned char* imageData =
                stbi_load_from_memory(
                    reinterpret_cast<
                    const unsigned char*
                    >(
                        embeddedTexture->pcData
                        ),

                    static_cast<int>(
                        embeddedTexture->mWidth
                        ),

                    &width,
                    &height,
                    &channels,
                    0
                );


            if (!imageData)
            {
                std::cerr
                    << "Failed to decode embedded texture."
                    << std::endl;

                return 0;
            }

            // imageData now contains decoded pixel data.
            // Create an OpenGL texture and upload these pixels to the GPU.
            unsigned int textureID =
                createTextureFromData(
                    imageData,
                    width,
                    height,
                    channels
                );

            // Free the decoded image data stored in CPU memory.
            // The texture itself has already been uploaded to GPU memory, so this CPU copy is no longer needed.
            stbi_image_free(
                imageData
            );


            return textureID;
        }


        std::cerr
            << "Uncompressed embedded texture "
            << "is not handled yet."
            << std::endl;

        return 0;
    }


    // -----------------------------------------
    // Case 2:
    // External texture file
    // The texture is NOT embedded.
    // such as:
    //
    // texture.png
    // skin.jpg
    // Common for .gltf files.
    // -----------------------------------------
    // 
    // Combine the model directory with the texture path.
    // Example:
    // directory:
    // assets/CesiumMan
    // texturePath:
    // CesiumMan_baseColor.png
    // fullPath:
    // assets/CesiumMan/CesiumMan_baseColor.png
    std::string fullPath =
        directory +
        "/" +
        texturePath.C_Str();


    int width = 0;
    int height = 0;
    int channels = 0;

    // Load and decode the external image file.
    unsigned char* imageData =
        stbi_load(
            fullPath.c_str(),
            &width,
            &height,
            &channels,
            0
        );


    if (!imageData)
    {
        std::cerr
            << "Failed to load external texture: "
            << fullPath
            << std::endl;

        return 0;
    }

    // Upload the decoded pixels to OpenGL.
    unsigned int textureID =
        createTextureFromData(
            imageData,
            width,
            height,
            channels
        );

    // Free CPU-side image data.
    stbi_image_free(
        imageData
    );

    // Return the OpenGL texture ID.
    return textureID;
}


//Upload the decoded pixel data to the GPU, create an OpenGL texture, and return the texture ID.
unsigned int Model::createTextureFromData(
    unsigned char* data,
    int width,
    int height,
    int channels
)
{
    unsigned int textureID = 0;
    // Create one texture object.
    // OpenGL will generate an ID and store it inside textureID.
    glGenTextures(
        1,
        &textureID
    );
    // Bind this texture as the current 2D texture.
    // All following texture operations will affect this texture object.
    glBindTexture(
        GL_TEXTURE_2D,
        textureID
    );


    // -----------------------------------------
    // Texture wrapping
    // -----------------------------------------
    
    // Define what happens when the U coordinate
    // goes outside the [0, 1] range.
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
		GL_REPEAT  // Repeat the texture horizontally
    );
    // Define what happens when the V coordinate
    // goes outside the [0, 1] range.
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
		GL_REPEAT // Repeat the texture vertically
    );


    // -----------------------------------------
    // Texture filtering
    // -----------------------------------------
    // Used when the texture appears smaller on screen.
    // GL_LINEAR_MIPMAP_LINEAR:
    // use mipmaps and linear interpolation.
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );
    // Used when the texture appears larger on screen.
    // GL_LINEAR:
    // smoothly interpolate neighboring texels.
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );


    // -----------------------------------------
    // Determine image format
    // -----------------------------------------

    GLenum format;
    // grayscale image.
    if (channels == 1)
    {
        format = GL_RED;
    }
    // RGB image.
    else if (channels == 3)
    {
        format = GL_RGB;
    }
    // RGBA image.
    else if (channels == 4)
    {
        format = GL_RGBA;
    }
    else
    {
        std::cerr
            << "Unsupported texture channel count: "
            << channels
            << std::endl;
        // Delete the texture object because we cannot upload this image format.
        glDeleteTextures(
            1,
            &textureID
        );

        return 0;
    }


    // -----------------------------------------
    // Upload image to GPU
    // -----------------------------------------

    glTexImage2D(
        GL_TEXTURE_2D, // Target: 2D texture
        0,              // Mipmap level 0: original full-resolution image
        format,         // How OpenGL stores the texture
        width,
        height,
        0,              // Border; must be 0
        format,         // Format of input pixel data
        GL_UNSIGNED_BYTE,  // Each color channel is stored as unsigned byte
        data            // Pointer to CPU pixel data
    );

    // Generate lower-resolution versions of the texture automatically.
    // These are used when the texture is viewed from farther away.
	// Generate mipmaps on the GPU for better performance and visual quality.
    glGenerateMipmap(
        GL_TEXTURE_2D
    );

    // Unbind the texture.
    // 0 means no texture is currently bound
    // to GL_TEXTURE_2D.
    glBindTexture(
        GL_TEXTURE_2D,
        0
    );


    return textureID;
}

// =====================================================
// Extract bone weights
// =====================================================

void Model::extractBoneWeightForVertices(
    std::vector<Vertex>& vertices,
    aiMesh* mesh
)
{
    // Loop through all bones in this mesh.
    for (
        unsigned int boneIndex = 0;
        boneIndex < mesh->mNumBones;
        ++boneIndex
        )
    {
        aiBone* bone =
            mesh->mBones[
                boneIndex
            ];


        std::string boneName =
            bone->mName.C_Str();


        int boneID = -1;

        // Register bone if it has not been seen before
        if (
            boneMapping.find(boneName)
            ==
            boneMapping.end()
            )
        {
            boneID =
                boneCount;


            // Store name -> ID mapping.
            boneMapping[
                boneName
            ] = boneID;


            BoneInfo info;


            // Store the bone offset matrix.
            info.offset =
                convertMatrixToGLM(
                    bone->mOffsetMatrix
                );


            boneInfo.push_back(
                info
            );


            boneCount++;
        }

        else
        {
            // Bone already exists.
            boneID =
                boneMapping[
                    boneName
                ];
        }


        // -------------------------------------------------
        // Assign this bone to influenced vertices
        // -------------------------------------------------

        for (
            unsigned int weightIndex = 0;
            weightIndex < bone->mNumWeights;
            ++weightIndex
            )
        {
            unsigned int vertexID =
                bone->mWeights[
                    weightIndex
                ].mVertexId;


            float weight =
                bone->mWeights[
                    weightIndex
                ].mWeight;


            // Safety check.
            if (
                vertexID >=
                vertices.size()
                )
            {
                continue;
            }


            // Find the first empty bone slot.
            for (
                int i = 0;
                i < MAX_BONE_INFLUENCE;
                ++i
                )
            {
                if (
                    vertices[
                        vertexID
                    ].boneIDs[i] == -1
                            )
                {
                    vertices[
                        vertexID
                    ].boneIDs[i] =
                            boneID;


                        vertices[
                            vertexID
                        ].weights[i] =
                                weight;


                            break;
                }
            }
        }
    }
}

// =====================================================
// Convert Assimp matrix to GLM matrix
// =====================================================

glm::mat4 Model::convertMatrixToGLM(
    const aiMatrix4x4& from
)
{
    glm::mat4 to;


    // GLM uses column-major indexing:
    // matrix[column][row]

    to[0][0] = from.a1;
    to[1][0] = from.a2;
    to[2][0] = from.a3;
    to[3][0] = from.a4;

    to[0][1] = from.b1;
    to[1][1] = from.b2;
    to[2][1] = from.b3;
    to[3][1] = from.b4;

    to[0][2] = from.c1;
    to[1][2] = from.c2;
    to[2][2] = from.c3;
    to[3][2] = from.c4;

    to[0][3] = from.d1;
    to[1][3] = from.d2;
    to[2][3] = from.d3;
    to[3][3] = from.d4;


    return to;
}

const std::map<std::string, int>&
Model::getBoneMapping() const
{
    return boneMapping;
}

const std::vector<BoneInfo>&
Model::getBoneInfo() const
{
    return boneInfo;
}