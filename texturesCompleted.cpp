#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_s.h>

#include <iostream>
#include <vector>

//Function declarations, otherwise the compiler won't know about functions below a given function

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void simpleBlurTexture(GLuint input, GLuint& output);
void halfTextureSize(GLuint input, GLuint& output);
void fastBlurTexture(GLuint input, GLuint& output);
void calculateKernel(int size);
void renderQuad();

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
unsigned int textureNum = 0;

//vector of textures to swap through. Will have to change paths to reflect your files
std::vector<textureData> textures = { textureData("resources/textures/container.jpg"),textureData("resources/textures/matrix.jpg") ,textureData("resources/textures/screenshot3.png") };

//Bools to track: which stages are being performed, whether an input has been
//pressed already, and if a change in behaviour has occurred
bool blur = false;
bool blurPressed = false;
bool blurDirty = true;
bool half = false;
bool halfPressed = false;
bool halfDirty = true;
bool fastBlur = false;
bool fastBlurPressed = false;
bool fastBlurDirty = true;

//vector of floats representing a 1 dimensional convolution kernel. Doesn't store reduntant (symmetrical) values
std::vector<float> kernel1DEfficient;

//Framebuffer used to capture resulting images
GLuint fb;
//Because blur operations are done separately in each dimension, a working texture is needed to capture
//the middle state. input->blurWorkingTexture->output
GLuint blurWorkingTexture = 0;

//Shader objects for the new shaders created. Note: I added a default constructor to the shader class
//to allow this usage
Shader simpleBlurShader;
Shader halfShader;
Shader fastBlurShader;

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

	// build and compile our shaders. Note: you will have to adjust your pathes to match your machine and files
	// ------------------------------------
	simpleBlurShader = Shader("C:\\Users\\John\\Desktop\\simpleBlur.vs", "C:\\Users\\John\\Desktop\\simpleBlur.fs");
	fastBlurShader = Shader("C:\\Users\\John\\Desktop\\fastBlur.vs", "C:\\Users\\John\\Desktop\\fastBlur.fs");
	halfShader = Shader("C:\\Users\\John\\Desktop\\half.vs", "C:\\Users\\John\\Desktop\\half.fs");
	Shader ourShader("4.1.texture.vs", "4.1.texture.fs");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		// positions          // texture coords
		 -1.0f,  -1.0f, 0.0f,   0.0f, 1.0f,
		 -1.0f, 1.0f, 0.0f,   0.0f, 0.0f, 
		1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 
		1.0f,  -1.0f, 0.0f,   1.0f, 1.0f  
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};
	//Create the framebuffer
	glGenFramebuffers(1, &fb);

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

	// load and create textures
	// -------------------------
	loadTextures();
	//Generate the kernel data. Size is adjustable. This could also be call dynamically if you want to adjust the size
	calculateKernel(7);

	//Textures to capture the blur and half results
	GLuint output = 0;
	GLuint halfOutput = 0;

	//Used to track delta time
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

		//Ugly ifs. Should really just create temp variables to track inputs and outputs of stages
		if (blur) {
			if (half) {
				halfTextureSize(textures[textureNum].m_handle, halfOutput);
				simpleBlurTexture(halfOutput, output);

				// render
				// ------
				glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				// bind Texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, output);

				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

				// render container
				ourShader.use();
				glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
			else {
				simpleBlurTexture(textures[textureNum].m_handle, output);

				// render
				// ------
				glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				// bind Texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, output);

				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

				// render container
				ourShader.use();
				glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}
		else if (fastBlur) {
			if (half) {
				halfTextureSize(textures[textureNum].m_handle, halfOutput);
				fastBlurTexture(halfOutput, output);

				// render
				// ------
				glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				// bind Texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, output);

				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

				// render container
				ourShader.use();
				glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
			else {
				fastBlurTexture(textures[textureNum].m_handle, output);

				// render
				// ------
				glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				// bind Texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, output);

				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

				// render container
				ourShader.use();
				glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}
		else {
			if (half) {
				halfTextureSize(textures[textureNum].m_handle, halfOutput);

				// render
				// ------
				glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				// bind Texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, halfOutput);

				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

				// render container
				ourShader.use();
				glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
			else {
				// render
				// ------
				glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				// bind Texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, textures[textureNum].m_handle);

				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

				// render container
				ourShader.use();
				glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}
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

	glDeleteTextures(1, &blurWorkingTexture);
	glDeleteTextures(1, &output);
	glDeleteTextures(1, &halfOutput);

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
	//if 0-2 is pressed switch texture and regenerate associated textures to handle differing resolutions
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	else if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
		textureNum = 0;
		blurDirty = true;
		halfDirty = true;
		fastBlurDirty = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		textureNum = 1;
		blurDirty = true;
		halfDirty = true;
		fastBlurDirty = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		textureNum = 2;
		blurDirty = true;
		halfDirty = true;
		fastBlurDirty = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_KP_0) == GLFW_PRESS) {
		textureNum = 0;
		blurDirty = true;
		halfDirty = true;
		fastBlurDirty = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_KP_1) == GLFW_PRESS) {
		textureNum = 1;
		blurDirty = true;
		halfDirty = true;
		fastBlurDirty = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS) {
		textureNum = 2;
		blurDirty = true;
		halfDirty = true;
		fastBlurDirty = true;
	}
	//toggle blur. Blur pressed is to avoid flickering back and forth
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
		if (!blurPressed) {
			blur = !blur;
			fastBlur = false;
			blurDirty = true;
			halfDirty = true;
			fastBlurDirty = true;
		}
		blurPressed = true;
	}
	else {
		blurPressed = false;
	}
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
		if (!halfPressed) {
			half = !half;
			blurDirty = true;
			halfDirty = true;
			fastBlurDirty = true;
		}
		halfPressed = true;
	}
	else {
		halfPressed = false;
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		if (!fastBlurPressed) {
			fastBlur = !fastBlur;
			blur = false;
			blurDirty = true;
			halfDirty = true;
			fastBlurDirty = true;
		}
		fastBlurPressed = true;
	}
	else {
		fastBlurPressed = false;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

//Performs our initial "simple" separable gaussian blur
//Input is the texture to be blurred and output is where the result is stored
void simpleBlurTexture(GLuint input, GLuint& output) {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	//if a change has occurred regenrate the textures
	if (blurDirty) {
		//if a texture has already been allocated, free it before proceeding
		if (output != 0) {
			glDeleteTextures(1, &output);
		}
		if (blurWorkingTexture != 0) {
			glDeleteTextures(1, &blurWorkingTexture);
		}
		glGenTextures(1, &output);
		glBindTexture(GL_TEXTURE_2D, output);
		//if half stage is happening then the resolution will be /2
		if (half) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[textureNum].m_size.x/2, textures[textureNum].m_size.y/2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[textureNum].m_size.x, textures[textureNum].m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenTextures(1, &blurWorkingTexture);
		glBindTexture(GL_TEXTURE_2D, blurWorkingTexture);

		if (half) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[textureNum].m_size.x / 2, textures[textureNum].m_size.y / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[textureNum].m_size.x, textures[textureNum].m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//bind blurworkingTexture to FB colour attachment
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurWorkingTexture, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
		blurDirty = false;
	}

	if (half) {
		glViewport(0, 0, textures[textureNum].m_size.x/2, textures[textureNum].m_size.y/2);
	}
	else {
		glViewport(0, 0, textures[textureNum].m_size.x, textures[textureNum].m_size.y);
	}

	GLuint temp = input;
	for (int i = 0; i < 5; i++) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, temp);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		simpleBlurShader.use();
		simpleBlurShader.setFloat("kernelSize", kernel1DEfficient.size());
		glUniform1fv(glGetUniformLocation(simpleBlurShader.ID, "weight"), kernel1DEfficient.size(), &kernel1DEfficient[0]);
		simpleBlurShader.setBool("horizontal", true);
		renderQuad();

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output, 0);
		glBindTexture(GL_TEXTURE_2D, blurWorkingTexture);
		simpleBlurShader.setBool("horizontal", false);
		renderQuad();
		temp = output;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//Creates a convolution kernel with the requested size and stores the one dimension
//version of it in kernel1DEfficient
//https://stackoverflow.com/questions/8204645/implementing-gaussian-blur-how-to-calculate-convolution-matrix-kernel
void calculateKernel(int size) {
	kernel1DEfficient.clear();
	double sigma = 1;
	std::vector<std::vector<float>> kernel(size, std::vector<float>(size, 0));
	float mean = int(size / 2);
	float sum = 0.0; // For accumulating the kernel values
	for (int x = 0; x < size; ++x)
		for (int y = 0; y < size; ++y) {
			kernel[x][y] = (float)sqrt(exp(-0.5 * (pow((x - mean) / sigma, 2.0) + pow((y - mean) / sigma, 2.0)))
				/ (2 * glm::pi<double>() * sigma * sigma));

			// Accumulate the kernel values
			sum += kernel[x][y];
		}

	// Normalize the kernel
	for (int x = 0; x < size; ++x)
		for (int y = 0; y < size; ++y)
			kernel[x][y] /= sum;

	for (int x = 0; x < size; x++) {
		float result = 0;
		for (int y = 0; y < size; y++) {
			result += kernel[x][y];
		}
		//Only store non repeating values
		if (x >= (int(size / 2))) {
			kernel1DEfficient.push_back(result);
		}
	}
}

//Performs a draw call of a simple quad
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

//Halves the size of a texture
//Input is the texture to be halved and output is where the result is stored
void halfTextureSize(GLuint input, GLuint& output) {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	if (halfDirty) {
		if (output != 0) {
			glDeleteTextures(1, &output);
		}
		glGenTextures(1, &output);
		glBindTexture(GL_TEXTURE_2D, output);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[textureNum].m_size.x / 2, textures[textureNum].m_size.y / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
		halfDirty = false;
	}

	glViewport(0, 0, textures[textureNum].m_size.x / 2, textures[textureNum].m_size.y / 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, input);
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	halfShader.use();
	renderQuad();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//Performs our more efficient hardware interpolated separable gaussian blur
//Input is the texture to be blurred and output is where the result is stored
void fastBlurTexture(GLuint input, GLuint& output) {

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	if (fastBlurDirty) {
		if (output != 0) {
			glDeleteTextures(1, &output);
		}
		if (blurWorkingTexture != 0) {
			glDeleteTextures(1, &blurWorkingTexture);
		}
		glGenTextures(1, &output);
		glBindTexture(GL_TEXTURE_2D, output);
		if (half) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[textureNum].m_size.x / 2, textures[textureNum].m_size.y / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[textureNum].m_size.x, textures[textureNum].m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenTextures(1, &blurWorkingTexture);
		glBindTexture(GL_TEXTURE_2D, blurWorkingTexture);

		if (half) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[textureNum].m_size.x / 2, textures[textureNum].m_size.y / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[textureNum].m_size.x, textures[textureNum].m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurWorkingTexture, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
		fastBlurDirty = false;
	}

	if (half) {
		glViewport(0, 0, textures[textureNum].m_size.x / 2, textures[textureNum].m_size.y / 2);
	}
	else {
		glViewport(0, 0, textures[textureNum].m_size.x, textures[textureNum].m_size.y);
	}

	GLuint temp = input;
	for (int i = 0; i < 5; i++) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, temp);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		fastBlurShader.use();
		fastBlurShader.setBool("horizontal", true);
		renderQuad();

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output, 0);
		glBindTexture(GL_TEXTURE_2D, blurWorkingTexture);
		fastBlurShader.setBool("horizontal", false);
		renderQuad();
		temp = output;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}