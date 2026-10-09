#version 330 core

// -------------------------------------------------
// Vertex attributes
// -------------------------------------------------

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// IDs of up to 4 bones affecting this vertex.
layout (location = 3) in ivec4 aBoneIDs;

// Corresponding influence weights.
layout (location = 4) in vec4 aWeights;


// -------------------------------------------------
// Data sent to fragment shader
// -------------------------------------------------

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;


// -------------------------------------------------
// Regular transformation matrices
// -------------------------------------------------

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


// -------------------------------------------------
// Skeleton
// -------------------------------------------------

const int MAX_BONES = 100;

// Final transformation matrix of every bone.
//
// Example:
// finalBonesMatrices[0] = transformation of bone 0
// finalBonesMatrices[1] = transformation of bone 1
// ...
uniform mat4 finalBonesMatrices[MAX_BONES];


void main()
{
    // Start with an empty skinned position.
    vec4 skinnedPosition =vec4(0.0);

     vec3 skinnedNormal =vec3(0.0);

    // Total weight actually used.
    float totalWeight = 0.0;


    // -------------------------------------------------
    // Apply up to 4 bone influences
    // -------------------------------------------------

    for (int i = 0; i < 4; ++i)
    {
        int boneID =
            aBoneIDs[i];


        // -1 means this bone slot is unused.
        if (boneID < 0)
        {
            continue;
        }


        // Safety check.
        if (boneID >= MAX_BONES)
        {
            continue;
        }


        float weight =
            aWeights[i];


        // Transform the original vertex position
        // using this bone's matrix.
        vec4 bonePosition =
            finalBonesMatrices[boneID] *
            vec4(aPos, 1.0);


        // Add this bone's weighted contribution.
        skinnedPosition +=
            bonePosition *
            weight;

        // Transform normal by the bone.
        //
        // We use mat3 because normals
        // do not need translation.
        vec3 boneNormal =
            mat3(
                finalBonesMatrices[boneID]
            ) *
            aNormal;


        // Add weighted normal contribution.
        skinnedNormal +=
            boneNormal *
            weight;

        totalWeight +=
            weight;
    }


    // -------------------------------------------------
    // Vertices without bone influences
    // -------------------------------------------------

    // If this vertex is not controlled by any bone,
    // simply use its original position.
    if (totalWeight <= 0.0)
    {
        skinnedPosition =vec4(aPos, 1.0);
        skinnedNormal =aNormal;
    }else
    {
        // Normalize in case weights do not sum
        // exactly to 1.0.
        skinnedPosition /= totalWeight;

        skinnedNormal /=totalWeight;
    }


    // -------------------------------------------------
    // Transform to world space
    // -------------------------------------------------

    vec4 worldPosition =
        model *
        skinnedPosition;


    FragPos =
        worldPosition.xyz;


    // For now we keep the original normal transformation.
    // Later we can also skin the normal using bone matrices.
    Normal =
        mat3(
            transpose(
                inverse(model)
            )
        ) *
        skinnedNormal;


    TexCoords =
        aTexCoords;


    // Final screen position.
    gl_Position =
        projection *
        view *
        worldPosition;
}