#pragma once

#include <string>
#include <vector>
#include <map>

#include <glm/glm.hpp>

#include <assimp/scene.h>
#include <assimp/matrix4x4.h>

#include "Mesh.h"
#include "Shader.h"

struct BoneInfo
{
    // Transform from mesh space to bone space.
    glm::mat4 offset;

    // Final transformation used later for animation.
    glm::mat4 finalTransformation;
    // Return the mapping from bone name to bone ID.
    
};
// Model class:
// Responsible for loading a complete 3D model from a file.
//
// A model can contain multiple meshes.
// For a virtual character, these meshes may represent:
//
// - body
// - head
// - hair
// - clothes
// - eyes
// - accessories
//
// Assimp reads the model file.
// We then convert Assimp's mesh data into our own Mesh objects.
class Model
{
public:
    // Constructor:
    // Load the model immediately from the given file path.
    Model(const std::string& path);
    
    // Draw every mesh contained in this model.
    void draw(Shader& shader);
    // Return the mapping from bone name to bone ID.
    const std::map<std::string, int>&
        getBoneMapping() const;

    // Return information for all bones.
    const std::vector<BoneInfo>&
        getBoneInfo() const;

private:
    // All meshes belonging to this model.
    std::vector<Mesh> meshes;

    // Directory containing the model file.
    //
    // This will later be useful for locating textures.
    std::string directory;

    // -------------------------------------------------
     // Skeleton information
     // -------------------------------------------------

     // Maps bone name -> bone ID.
     //
     // Example:
     // "UpperArm" -> 3
     // "Forearm"  -> 4
    std::map<std::string, int> boneMapping;


    // Stores information for every bone.
    std::vector<BoneInfo> boneInfo;

  
    // Number of bones currently registered.
    int boneCount = 0;

    // -------------------------------------------------
    // Model loading
    // -------------------------------------------------
    
    void loadModel(const std::string& path);

    // Recursively process Assimp's scene hierarchy.
    void processNode(
        aiNode* node,
        const aiScene* scene
    );

    // Convert one Assimp mesh into our own Mesh object.
    Mesh processMesh(
        aiMesh* mesh,
        const aiScene* scene
    );
    // -------------------------------------------------
   // Texture loading
   // -------------------------------------------------
    // Load one texture from an Assimp material.
    // Supports both embedded textures and external texture files.
    unsigned int loadMaterialTexture(
        aiMaterial* material,
        aiTextureType type,
        const aiScene* scene
    );

    // Create an OpenGL texture from decoded image data.
    unsigned int createTextureFromData(
        unsigned char* data,
        int width,
        int height,
        int channels
    );
    
    // -------------------------------------------------
    // Bone loading
    // -------------------------------------------------

    // Read bone IDs and weights from Assimp
    // and assign them to vertices.
    void extractBoneWeightForVertices(
        std::vector<Vertex>& vertices,
        aiMesh* mesh
    );
    // Convert an Assimp matrix to a GLM matrix.
    glm::mat4 convertMatrixToGLM(
        const aiMatrix4x4& from
    );

    
};