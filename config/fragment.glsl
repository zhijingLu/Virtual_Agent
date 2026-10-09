#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D diffuseTexture;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
    // Read the original texture color.
    vec3 objectColor =
        texture(
            diffuseTexture,
            TexCoords
        ).rgb;


    // -----------------------------
    // Ambient lighting
    // -----------------------------

    float ambientStrength =
        0.25;

    vec3 ambient =
        ambientStrength *
        lightColor;


    // -----------------------------
    // Diffuse lighting
    // -----------------------------

    vec3 norm =
        normalize(Normal);

    vec3 lightDir =
        normalize(
            lightPos - FragPos
        );

    float diff =
        max(
            dot(norm, lightDir),
            0.0
        );

    vec3 diffuse =
        diff *
        lightColor;

    // ---------------------------------
    // Specular lighting 高光
    // ---------------------------------

    float specularStrength = 0.5;

    // Direction from fragment to camera.
    vec3 viewDir =
        normalize(
            viewPos - FragPos
        );

    // Reflection direction of the light.
    vec3 reflectDir =
        reflect(
            -lightDir,
            norm
        );

    float spec =
        pow(
            max(
                dot(viewDir, reflectDir),
                0.0
            ),
            32.0
        );

    vec3 specular =
        specularStrength *
        spec *
        lightColor;

    // -----------------------------
    // Final color
    // -----------------------------

    vec3 result =
        (
            ambient +
            diffuse +
            specular
        ) *
        objectColor;

    FragColor =
        vec4(
            result,
            1.0
        );
}