#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "shader.h"
#include "camera.h"

#include <iostream>

#include "cylinder.h"
#include "Sphere.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void createCube(unsigned int& vbo, unsigned int& vao, float posX, float posY, float posZ);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
unsigned int loadTexture(const char* path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const char* WINDOW_TITLE = "David France Final Project";

// camera
Camera camera(glm::vec3(-0.75f, 0.5f, 0.75f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// projection matrix
glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
bool perspective = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// sphere
GLuint sphereNumIndices;
GLuint sphereVertexArrayObjectID;
GLuint sphereIndexByteOffset;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// Flip images for texturing
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
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
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Register key, mouse, and window callbacks
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, keyCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader zprogram
	// ------------------------------------
	Shader lightingShader("shaderfiles/6.multiple_lights.vs", "shaderfiles/6.multiple_lights.fs");
	Shader lightCubeShader("shaderfiles/6.light_cube.vs", "shaderfiles/6.light_cube.fs");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float tableVertices[] = {
		-0.5f, 0.0f, -0.5f,  0.0f,  0.0f, 1.0f,  0.0f,  1.0f,
		 0.5f, 0.0f, -0.5f,  0.0f,  0.0f, 1.0f,  1.0f,  1.0f,
		 0.5f, 0.0f,  0.5f,  0.0f,  0.0f, 1.0f,  1.0f,  0.0f,
		 0.5f, 0.0f,  0.5f,  0.0f,  0.0f, 1.0f,  1.0f,  0.0f,
		-0.5f, 0.0f,  0.5f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f,
		-0.5f, 0.0f, -0.5f,  0.0f,  0.0f, 1.0f,  0.0f,  1.0f,
	};
	// Table is positioned at the center
	glm::vec3 tablePosition = glm::vec3(0.0f, 0.0f, 0.0f);

	// Positions of the point lights
	glm::vec3 pointLightPositions[] = {
		glm::vec3(-1.5f,  1.0f,  0.0f),
		glm::vec3(-1.5f,  1.0f,  3.0f),
		glm::vec3(-1.5f,  1.0f, -4.5f)
	};
	// Configure the table's VAO (and VBO)
	unsigned int tableVBO, tableVAO;
	glGenVertexArrays(1, &tableVAO);
	glGenBuffers(1, &tableVBO);

	glBindBuffer(GL_ARRAY_BUFFER, tableVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tableVertices), tableVertices, GL_STATIC_DRAW);

	glBindVertexArray(tableVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);


	// Set up cutting board VAO, VBO, and position
	glm::vec3 cuttingBoardPosition = glm::vec3(0.0f, 0.001f, 0.0f);
	unsigned int cuttingBoardVBO, cuttingBoardVAO;
	createCube(cuttingBoardVBO, cuttingBoardVAO, 0.4f, 0.075f, 0.6f);

	// Set up cheese block VAO, VBO, and position
	glm::vec3 cheeseBlockPosition = glm::vec3(0.0f, 0.024f, -0.053f);
	unsigned int cheeseBlockVBO, cheeseBlockVAO;
	createCube(cheeseBlockVBO, cheeseBlockVAO, 0.15f, 0.25f, 0.125f);

	// Set up cheese slice VAO, VBO, and positionglm::vec3 
	glm::vec3 cheeseSlicePosition = glm::vec3(0.0f, 0.024f, 0.053f);
	unsigned int cheeseSliceVBO, cheeseSliceVAO;
	createCube(cheeseSliceVBO, cheeseSliceVAO, 0.15f, 0.025f, 0.125f);

	// Set up positions for eggs
	glm::vec3 eggPositions[] = {
		glm::vec3(-0.33f, 0.036f, 0.23f),
		glm::vec3(-0.35f, 0.036f, 0.10f),
		glm::vec3(-0.25f, 0.036f, 0.13f)
	};


	// Create egg (sphere)
	unsigned int eggVBO, eggVAO;
	Sphere S(1, 30, 30);
	glGenVertexArrays(1, &eggVAO);
	glGenBuffers(1, &eggVBO);
	glBindVertexArray(eggVAO);
	glBindBuffer(GL_ARRAY_BUFFER, eggVBO);

	// Create bowl (empty cylinder
	glm::vec3 bowlPosition = glm::vec3(-0.05f, 0.034f, 0.35f);
	static_meshes_3D::Cylinder C(2, 100, 1, true, true, true);


	// Configure the light's VAO (VBO stays the same - lights will be represented as a plane)
	unsigned int lightCubeVAO;
	glGenVertexArrays(1, &lightCubeVAO);
	glBindVertexArray(lightCubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, tableVBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Load textures - all images are my original pictures
	unsigned int planeTexture = loadTexture("images/table.jpg");
	unsigned int cuttingBoardTexture = loadTexture("images/cutting_board.jpg");
	unsigned int cheeseTexture = loadTexture("images/cheese_slice.jpg");
	unsigned int whiteEggTexture = loadTexture("images/white_egg1.jpg");
	unsigned int greenEggTexture = loadTexture("images/green_egg.jpg");
	unsigned int brownEggTexture = loadTexture("images/brown_egg.jpg");
	unsigned int bowlTexture = loadTexture("images/bowl2.jpg");

	// Egg texture array
	unsigned int eggTextures[] = {
		brownEggTexture,
		whiteEggTexture,
		greenEggTexture
	};

	// shader configuration
	// --------------------
	lightingShader.use();
	lightingShader.setInt("material.diffuse", 0);
	lightingShader.setInt("material.specular", 1);


	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Activate shader when setting uniforms/drawing objects
		lightingShader.use();
		lightingShader.setVec3("viewPos", camera.Position);
		lightingShader.setFloat("material.shininess", 32.0f);

		// Set up three point lights and one spotlight to represent distant point lights with one overhead light
		// point light 1 - color - white
		lightingShader.setVec3("pointLights[0].position", pointLightPositions[0]);
		lightingShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
		lightingShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[0].constant", 1.0f);
		lightingShader.setFloat("pointLights[0].linear", 0.09);
		lightingShader.setFloat("pointLights[0].quadratic", 0.032);
		// point light 2 - color - soft yellow/white
		lightingShader.setVec3("pointLights[1].position", pointLightPositions[1]);
		lightingShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
		lightingShader.setVec3("pointLights[1].specular", 0.9f, 1.0f, 0.8f);
		lightingShader.setFloat("pointLights[1].constant", 1.0f);
		lightingShader.setFloat("pointLights[1].linear", 0.09);
		lightingShader.setFloat("pointLights[1].quadratic", 0.032);
		// point light 3 - color - white
		lightingShader.setVec3("pointLights[2].position", pointLightPositions[2]);
		lightingShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
		lightingShader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[2].constant", 1.0f);
		lightingShader.setFloat("pointLights[2].linear", 0.09);
		lightingShader.setFloat("pointLights[2].quadratic", 0.032);

		// spotLight - position fixed on top of the scene pointing down - color - soft yellow/white
		lightingShader.setVec3("spotLight.position", 0.1f, 2.0f, 0.1f);
		lightingShader.setVec3("spotLight.direction", 0.0f, -1.0f, 0.0f);
		lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
		lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
		lightingShader.setVec3("spotLight.specular", 0.9f, 1.0f, 0.8f);
		lightingShader.setFloat("spotLight.constant", 1.0f);
		lightingShader.setFloat("spotLight.linear", 0.09);
		lightingShader.setFloat("spotLight.quadratic", 0.032);
		lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
		lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

		// view/projection transformations
		//glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		lightingShader.setMat4("projection", projection);
		lightingShader.setMat4("view", view);

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);
		lightingShader.setMat4("model", model);

		// Draw table
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, planeTexture);
		glBindVertexArray(tableVAO);
		// calculate the model matrix for each object and pass it to shader before drawing
		model = glm::mat4(1.0f);
		model = glm::translate(model, tablePosition);
		model = glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		lightingShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);

		// Draw cutting board
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cuttingBoardTexture);
		glBindVertexArray(cuttingBoardVAO);
		model = glm::mat4(1.0f);
		model = glm::translate(model, cuttingBoardPosition);
		model = glm::scale(model, glm::vec3(0.3f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		lightingShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);

		// Draw cheese block
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cheeseTexture);
		glBindVertexArray(cheeseBlockVAO);
		model = glm::mat4(1.0f);
		model = glm::translate(model, cheeseBlockPosition);
		model = glm::scale(model, glm::vec3(0.3f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		lightingShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);

		// Draw cheese slice
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cheeseTexture);
		glBindVertexArray(cheeseSliceVAO);
		model = glm::mat4(1.0f);
		model = glm::translate(model, cheeseSlicePosition);
		model = glm::scale(model, glm::vec3(0.3f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		lightingShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);
		
		// Draw eggs
		glBindVertexArray(eggVAO);
		for (unsigned int i = 0; i < 3; i++)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, eggTextures[i]);
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(eggPositions[i]));
			model = glm::scale(model, glm::vec3(0.035f));
			model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			lightingShader.setMat4("model", model);

			S.Draw();
		}

		// Draw bowl
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bowlTexture);
		glBindVertexArray(eggVAO);
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(bowlPosition));
		model = glm::scale(model, glm::vec3(0.045f));
		lightingShader.setMat4("model", model);
		C.render();

		// also draw the lamp object(s)
		lightCubeShader.use();
		lightCubeShader.setMat4("projection", projection);
		lightCubeShader.setMat4("view", view);

		// we now draw as many light bulbs as we have point lights.
		glBindVertexArray(lightCubeVAO);
		for (unsigned int i = 0; i < 3; i++)
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
			lightCubeShader.setMat4("model", model);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &tableVAO);
	glDeleteVertexArrays(1, &lightCubeVAO);
	glDeleteBuffers(1, &tableVBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// Process keyboard input
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
	
}

// Key callback method for keystrokes that need to be handle once per press, not every frame, 'P' key in this case
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		if (perspective)
		{
			projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
			perspective = false;
			std::cout << "P pressed" << std::endl;
		}
		else
		{
			projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
			perspective = true;
			std::cout << "P pressed" << std::endl;
		}
	}
}

// Process window size changes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// Process mouse movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// Process mouse wheel scroll - speeds up and slows down camera speed
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		flipImageVertically(data, width, height, nrComponents);

		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}


// Create vbo and vao for cube shaped objects 
void createCube(unsigned int& vbo, unsigned int& vao, float posX, float posY, float posZ) {
	float negX = -1.0 * posX;
	float negZ = -1.0 * posZ;

	// Vertex Data
	float verts[] = {
		// positions          // normals           // texture coords
		negX,  0.0f,  negZ,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		posX,  0.0f,  negZ,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		posX,  posY,  negZ,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		posX,  posY,  negZ,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		negX,  posY,  negZ,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
		negX,  0.0f,  negZ,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

		negX,  0.0f,  posZ,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		posX,  0.0f,  posZ,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		posX,  posY,  posZ,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		posX,  posY,  posZ,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		negX,  posY,  posZ,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
		negX,  0.0f,  posZ,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

		negX,  posY,  posZ, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		negX,  posY,  negZ, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		negX,  0.0f,  negZ, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		negX,  0.0f,  negZ, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		negX,  0.0f,  posZ, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		negX,  posY,  posZ, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		posX,  posY,  posZ,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		posX,  posY,  negZ,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		posX,  0.0f,  negZ,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		posX,  0.0f,  negZ,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		posX,  0.0f,  posZ,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		posX,  posY,  posZ,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		negX,  0.0f,  negZ,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
		posX,  0.0f,  negZ,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		posX,  0.0f,  posZ,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		posX,  0.0f,  posZ,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		negX,  0.0f,  posZ,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		negX,  0.0f,  negZ,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		negX,  posY,  negZ,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		posX,  posY,  negZ,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
		posX,  posY,  posZ,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		posX,  posY,  posZ,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		negX,  posY,  posZ,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		negX,  posY,  negZ,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f
	};

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	glBindVertexArray(vao);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
}