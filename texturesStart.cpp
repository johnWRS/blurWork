#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_s.h>

#include <iostream>
#include <vector>

//Function declarations, otherwise the compiler won't know about functions below a given function

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

//Class to store information about a given texture
class textureData
{
public:
    textureData(const std::string& path) { m_filepath = path; }
    std::string m_filepath;
    GLuint m_handle = -1;
    glm::vec2 m_size = glm::vec2(0, 0);
    int m_channels = 0;
};

//currently selected texture index
unsigned int texture = 0;
//vector of textures to swap through. Will have to change paths to reflect your files
std::vector<textureData> textures = { textureData("resources/textures/container.jpg"),textureData("resources/textures/matrix.jpg") ,textureData("resources/textures/screenshot3.png") };

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

//Iterates through over textures loading files and setting member data
void loadTextures() {
    int width;
    int height;
    for (textureData& text : textures) {
        glGenTextures(1, &text.m_handle);
        glBindTexture(GL_TEXTURE_2D, text.m_handle); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // load image, create texture and generate mipmaps
        // The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
        unsigned char* data = stbi_load(FileSystem::getPath(text.m_filepath).c_str(), &width, &height, &text.m_channels, 0);
        if (data)
        {
            text.m_size = glm::vec2(width, height);
            //depending on number of channels allocate RGB or RGBA. Could be expanded for more encodings too
            if (text.m_channels == 3) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, text.m_size.x, text.m_size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            }
            else if (text.m_channels == 4) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, text.m_size.x, text.m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            }
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            std::cout << "Failed to load texture" << std::endl;
        }
        //Free image data from CPU side after GPU resource has been allocated
        stbi_image_free(data);
    }
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("4.1.texture.vs", "4.1.texture.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // texture coords
        -1.0f, -1.0f, 0.0f,   0.0f, 1.0f,
        -1.0f, 1.0f,  0.0f,   0.0f, 0.0f,
         1.0f, 1.0f,  0.0f,   1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,   1.0f, 1.0f
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // load and create texture s
    // -------------------------
    loadTextures();

    double time = glfwGetTime();
    double pastTime = time;
    std::cout << "FPS:" << std::endl;
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // bind Texture
        glBindTexture(GL_TEXTURE_2D, textures[texture].m_handle);

        // render container
        ourShader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
        pastTime = time;
        time = glfwGetTime();
        std::cout << "\r" << 1 / (time - pastTime);
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    for (int i = 0; i < textures.size(); i++) {
        glDeleteTextures(1, &textures[i].m_handle);
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    else if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
        texture = 0;
    else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        texture = 1;
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        texture = 2;
    else if (glfwGetKey(window, GLFW_KEY_KP_0) == GLFW_PRESS)
        texture = 0;
    else if (glfwGetKey(window, GLFW_KEY_KP_1) == GLFW_PRESS)
        texture = 1;
    else if (glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS)
        texture = 2;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
