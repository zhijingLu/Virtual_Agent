#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

#include "character/Shader.h"
#include "character/Model.h"
#include "character/Camera.h"
//#include "animation/Animation.h"
//#include "animation/Animator.h"
#include "animation/AnimationController.h"
//speech related files

#include "character/CharacterController.h"

//#include "input/ConsoleInput.h"
#include "input/SpeechRecognizer.h"
#include "input/AudioRecorder.h"
//vcpkg install --triplet x64-windows-static
int main()
{
    // -------------------------------------------------
    // 1. Initialize GLFW
    // -------------------------------------------------

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    // Request OpenGL 3.3 Core Profile.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );


    // -------------------------------------------------
    // 2. Create window
    // -------------------------------------------------

    GLFWwindow* window = glfwCreateWindow(
        800,
        600,
        "Virtual Character",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "Failed to create GLFW window." << std::endl;

        glfwTerminate();
        return -1;
    }

    // Make this window's OpenGL context current.
    glfwMakeContextCurrent(window);


    // -------------------------------------------------
    // 3. Initialize GLAD
    // -------------------------------------------------

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD." << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    std::cout
        << "OpenGL Version: "
        << glGetString(GL_VERSION)
        << std::endl;


    // -------------------------------------------------
    // 4. Enable depth testing
    // -------------------------------------------------

    // Required for correct 3D rendering.
    // Without depth testing, surfaces behind the character
    // may incorrectly appear in front.
    glEnable(GL_DEPTH_TEST);


    // -------------------------------------------------
    // 5. Create shader
    // -------------------------------------------------

    Shader shader(
        "config/vertex.glsl",
        "config/fragment.glsl"
    );


    // -------------------------------------------------
    // 6. Load character model
    // -------------------------------------------------
    
    float animationSpeed = 2.0f;

    // Assimp will load the model file.
    //
    // Make sure the file actually exists at this path.
    Model character(
        "assets/Lu.glb"
    );

    // Load the first animation from the same model file.
    AnimationController animationController(
        "assets/Lu.glb",
        &character
    );
   
    


    // -------------------------------------------------
    // 7. Create camera
    // -------------------------------------------------

    Camera camera;


    // -------------------------------------------------
    // 8. Timing variables
    // -------------------------------------------------

    // deltaTime represents the time between two frames.
    // This makes movement independent of frame rate.
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;


    // -------------------------------------------------
    // 9. Set background color
    // -------------------------------------------------

    glClearColor(
        0.15f,
        0.18f,
        0.22f,
        1.0f
    );

    // -------------------------------------------------
    // Speech system
    // -------------------------------------------------
   

    CharacterController characterController(
        &animationController
    );
    bool speechKeyPressed = false;
    bool speechActive = false;

    // -------------------------------------------------
	// thread for console input
    // -------------------------------------------------
    //   ConsoleInput consoleInput;

    //   consoleInput.start();
    // -------------------------------------------------
    // Speech recognition
    // -------------------------------------------------

    SpeechRecognizer speechRecognizer;

    speechRecognizer.start();
    // -------------------------------------------------
    // Microphone
    // -------------------------------------------------

    AudioRecorder audioRecorder;
    bool microphoneStarted =
        audioRecorder.start(
            [&speechRecognizer](const AudioBuffer& chunk)
            {
                speechRecognizer.processAudioChunk(chunk);
            }
        );


    if (!microphoneStarted)
    {
        std::cerr
            << "Failed to start microphone."
            << std::endl;

        return -1;
    }

    // -------------------------------------------------
    // 10. Main render loop
    // -------------------------------------------------

   

    while (!glfwWindowShouldClose(window))
    {
        // ---------------------------------------------
        // Calculate delta time
        // ---------------------------------------------

        float currentFrame =
            static_cast<float>(glfwGetTime());

        deltaTime =
            currentFrame - lastFrame;

        lastFrame =
            currentFrame;


      
        //if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        //{
        //    camera.moveForward(deltaTime);
        //}

        //// Move camera backward.
        //if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        //{
        //    camera.moveBackward(deltaTime);
        //}

        //// Move camera left.
        //if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        //{
        //    camera.moveLeft(deltaTime);
        //}

        //// Move camera right.
        //if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        //{
        //    camera.moveRight(deltaTime);
        //}

        //// Press ESC to close the window.
        //if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        //{
        //    glfwSetWindowShouldClose(
        //        window,
        //        true
        //    );
        //}


        //
        //
        //if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        //{
        //    characterController.setState(
        //        CharacterState::Idle
        //    );
        //}

        //if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        //{
        //    characterController.setState(
        //        CharacterState::Walk
        //    );
        //}

        //if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        //{
        //    characterController.setState(
        //        CharacterState::Run
        //    );
        //}

        //if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        //{
        //    characterController.setState(
        //        CharacterState::Talk
        //    );
        //}

        //if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
        //{
        //    characterController.setState(
        //        CharacterState::StandToSit
        //    );
        //}

        //if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
        //{
        //    characterController.setState(
        //        CharacterState::Sitting
        //    );
        //}
        //if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        //{
        //    animationController.playSequence(
        //        {
        //            { AnimationState::Idle, 1 },
        //            { AnimationState::Walk, 4 },
        //            { AnimationState::Idle, 1 },
        //            { AnimationState::Run, 3 }
        //        }
        //    );
        //}
        // -------------------------------------------------
        // Press T to make the character speak.
        // -------------------------------------------------

        //if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        //{
        //    // Trigger only once per key press.
        //    if (!speechKeyPressed)
        //    {
        //        speechKeyPressed = true;

        //        std::string text =
        //            "Hello, I am your virtual character，zhijing Lu. I am here to help you. What can I do for you? I am in aachen now, do you want to make a travel?";

        //   
        //        characterController.speak(text);
        //        
        //    }
        //}
        //else
        //{
        //    // Key released, allow another trigger.
        //    speechKeyPressed = false;
        //}
       

        //std::string userInput;
        //if (consoleInput.tryPop(userInput))
        //{
        //    std::cout
        //        << "Received: "
        //        << userInput
        //        << std::endl;

        //    characterController.speak(userInput);
        //}
        // -------------------------------------------------
        // Check Whisper result
        // -------------------------------------------------

        std::string recognizedText;

        if (
            speechRecognizer.tryGetText(
                recognizedText
            )
        )
        {
            std::cout
                << "User: "
                << recognizedText
                << std::endl;


            // Temporary test:
            // Let Lily repeat what you said.
            characterController.speak(
                recognizedText
            );
        }

        // -----------------------------------------
        // Update animation
        // -----------------------------------------
        characterController.update(
            deltaTime
        );
        //animator.updateAnimation(
        //    deltaTime
        //);
        /* if (animationPlaying)
        {
            animator.updateAnimation(
                deltaTime* animationSpeed
            );
        }*/
        // ---------------------------------------------
        // Clear previous frame
        // ---------------------------------------------
        glClearColor(
            0.15f,
            0.18f,
            0.22f,
            1.0f
        );
        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );
        // ---------------------------------------------
        // Activate shader
        // ---------------------------------------------

        shader.use();
        // Bind the diffuse sampler to texture unit 0.
        shader.setInt("diffuseTexture", 0);
        // -----------------------------------------
        // Lighting
        // -----------------------------------------
        // Position of the light in world space.
        shader.setVec3(
            "lightPos",
            glm::vec3(
                2.0f,
                3.0f,
                3.0f
            )
        );

        // White light.
        shader.setVec3(
            "lightColor",
            glm::vec3(
                1.0f,
                1.0f,
                1.0f
            )
        );
        // Send camera position to the shader.
        shader.setVec3(
            "viewPos",
            camera.position
        );
        
        // ---------------------------------------------
        // Model matrix
        // ---------------------------------------------

        // Start with identity matrix.
        glm::mat4 model =
            glm::mat4(1.0f);

        // Optional:
        // Move the character downward slightly.
        //
        // Useful if the model origin is around the hips
        // instead of the feet.
        model = glm::translate(
            model,
            glm::vec3(
                0.0f,
                -10.0f,
                0.0f
            )
        );
        model = glm::rotate(
            model,
            glm::radians(90.0f),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );
        

        // Optional:
        // Scale the character.
        //
        // Some imported models are extremely large or small.
        model = glm::scale(
            model,
            glm::vec3(
                0.2f,
                0.2f,
                0.2f
            )
        );


        // ---------------------------------------------
        // View matrix
        // ---------------------------------------------

        // Camera calculates the view matrix based on:
        // position
        // front direction
        // up direction
        glm::mat4 view =
            camera.getViewMatrix();


        // ---------------------------------------------
        // Projection matrix
        // ---------------------------------------------

        glm::mat4 projection =
            glm::perspective(
                glm::radians(45.0f), // field of view
                800.0f / 600.0f,    // aspect ratio
                0.1f,               // near clipping plane
                100.0f              // far clipping plane
            );


        // ---------------------------------------------
        // Send matrices to shader
        // ---------------------------------------------

        shader.setMat4(
            "model",
            model
        );

        shader.setMat4(
            "view",
            view
        );

        shader.setMat4(
            "projection",
            projection
        );

        // -----------------------------------------
        // Send bone matrices to GPU
        // -----------------------------------------

        const std::vector<glm::mat4>&
            boneMatrices =
            animationController
            .getFinalBoneMatrices();


        for (
            int i = 0;
            i < static_cast<int>(
                boneMatrices.size()
                );
            ++i
            )
        {
            shader.setMat4(
                "finalBonesMatrices[" +
                std::to_string(i) +
                "]",
                boneMatrices[i]
            );
        }

        // ---------------------------------------------
        // Draw character
        // ---------------------------------------------

        character.draw(shader);


        // ---------------------------------------------
        // Display rendered frame
        // ---------------------------------------------

        glfwSwapBuffers(window);


        // ---------------------------------------------
        // Process GLFW events
        // ---------------------------------------------

        glfwPollEvents();
    }


    // -------------------------------------------------
    // 11. Clean up
    // -------------------------------------------------
    //consoleInput.stop();
    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}