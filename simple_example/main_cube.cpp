#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Mesh.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"


int main()
{

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
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
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // -------------------------------------------------
    // 2. Create a window
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

    // GLAD loads OpenGL functions through GLFW.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    // -------------------------------------------------
   // 4. Enable depth testing
   // -------------------------------------------------
    // Required for correct 3D occlusion.
    glEnable(GL_DEPTH_TEST);
   
    
    Shader shader(
        "config/vertex.glsl",
        "config/fragment.glsl"
    );

    Mesh mesh;

    glClearColor(
        0.2f,
        0.3f,
        0.4f,
        1.0f
    );

    Camera camera;
    

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
        // ---------------------------------------------
        // Process keyboard input
        // ---------------------------------------------

        // Move camera forward.
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            camera.moveForward(deltaTime);
        }

        // Move camera backward.
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera.moveBackward(deltaTime);
        }

        // Move camera left.
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera.moveLeft(deltaTime);
        }

        // Move camera right.
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera.moveRight(deltaTime);
        }

        // Press ESC to close the window.
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }


        // Clear both color and depth buffers.
        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );
        // Activate shader program.
        shader.use();
		//------------------------------------------------- 
        // glm is a header-only library for OpenGL mathematics.
        // It provides classes and functions for vectors, matrices, and transformations.
        //------------------------------------------------- 
        //object itself 
        glm::mat4 model = glm::mat4(1.0f);
        // Rotate the cube.
        model = glm::rotate(
            model,
            glm::radians(30.0f),
            glm::vec3(1.0f, 1.0f, 0.0f)
        );
		  //camera view matrix
          //      glm::mat4 view = glm::mat4(1.0f);

          //      // Move the world backwards so that the cube appears in front of the camera. 
		        //// This is equivalent to moving the camera forward on z-axis.
          //      view = glm::translate(
          //          view,
          //          glm::vec3(0.0f, 0.0f, -3.0f)
          //      );
        glm::mat4 view = camera.getViewMatrix();
		// Create a perspective projection matrix. 3D objects to 2d
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), // field of view
            800.0f / 600.0f,    // aspect ratio
            0.1f,               // near plane  view frustum
            100.0f              // far plane
        );
        
        // -------------------------------------------------
        // Send matrices to the shader
        // -------------------------------------------------

        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        mesh.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    


    // -------------------------------------------------
    // 12. Clean up resources
    // -------------------------------------------------

    
 
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}