#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION 
#include <stb_image.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <CSVReader.h>
#include <Shader.h>
#include <iostream>
#include <vector>
#include <camera.h>

typedef struct
{
	unsigned int VAO;
	unsigned int VBO;
	unsigned int texture;
	int points;
	bool drawTexture;
} CustomObject;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
CustomObject load_custom_object(std::pair<std::string, std::string> fileNameAndTexture);
unsigned int load_object_texture(std::string textureFileName);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const unsigned int VERTICE_DEFINITION = 11; //3 Positions + 3 Colors + 3 Normal Vector + 2 Texture Coordinates

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 6.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// specular reflex
float specularStrength = 0.5;

int main()
{
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CSV Renderer", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// glew: load all OpenGL function pointers
	if (glewInit() != GLEW_OK) {
		std::cout << "An error occurred while starting GLEW!" << std::endl;
	}
	else {
		std::cout << "GLEW OK!" << std::endl;
		std::cout << glGetString(GL_VERSION) << std::endl;
	}

	// configure global opengl state
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader zprogram
	Shader lightingShader("src/shaders/phong_lighting.vs", "src/shaders/phong_lighting.fs");

	Shader lightCubeShader("src/shaders/light_cube.vs", "src/shaders/light_cube.fs");

	std::pair<std::string, std::string> modelsAndTextures[] =
	{
		{"src/resources/garden.csv", "src/textures/grass.jpg"},
		{"src/resources/walls.csv", "src/textures/wall.jpg"},
		{"src/resources/door.csv", "src/textures/door.jpg"},
		{"src/resources/window.csv", "src/textures/window.jpg"},
		{"src/resources/ceiling.csv", "src/textures/ceiling.jpg"},
		{"src/resources/rooftop.csv", "src/textures/rooftop.jpg"}
	};

	int modelsAndTexturesCount = sizeof(modelsAndTextures) / sizeof(modelsAndTextures[0]);
	CustomObject* customObjects = new CustomObject[modelsAndTexturesCount];
	for (int i = 0; i < modelsAndTexturesCount; i++)
		customObjects[i] = load_custom_object(modelsAndTextures[i]);

	CustomObject sun = load_custom_object({ "src/resources/sun.csv", "" });

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		processInput(window);

		// render
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// change the light's position values over time (can be done anywhere in the render loop actually, but try to do it at least before using the light source positions)
		lightPos.x = sin(glfwGetTime()) * 2.0f;
		lightPos.y = sin(glfwGetTime()) * 2.0f;
		lightPos.z = cos(glfwGetTime()) * 2.0f;

		// be sure to activate shader when setting uniforms/drawing objects
		lightingShader.use();
		lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
		lightingShader.setVec3("lightPos", lightPos);
		lightingShader.setVec3("viewPos", camera.Position);
		lightingShader.setFloat("specularStrength", specularStrength);

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		lightingShader.setMat4("projection", projection);
		lightingShader.setMat4("view", view);

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);
		lightingShader.setMat4("model", model);

		// render objects
		for (int i = 0; i < modelsAndTexturesCount; i++)
		{
			glBindTexture(GL_TEXTURE_2D, customObjects[i].texture);
			lightingShader.setBool("drawTexture", customObjects[i].drawTexture);
			glBindVertexArray(customObjects[i].VAO);
			glDrawArrays(GL_TRIANGLES, 0, customObjects[i].points);
		}

		// also draw the lamp object
		lightCubeShader.use();
		lightCubeShader.setMat4("projection", projection);
		lightCubeShader.setMat4("view", view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
		lightCubeShader.setMat4("model", model);

		glBindVertexArray(sun.VAO);
		glDrawArrays(GL_TRIANGLES, 0, sun.points);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	for (int i = 0; i < modelsAndTexturesCount; i++)
	{
		glDeleteVertexArrays(1, &customObjects[i].VAO);
		glDeleteBuffers(1, &customObjects[i].VBO);
	}

	glDeleteVertexArrays(1, &sun.VAO);
	glDeleteBuffers(1, &sun.VBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
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
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
	{
		specularStrength += 0.01f;
		if (specularStrength > 5.0f)
			specularStrength = 5.0f;
		std::cout << "Especular = " << specularStrength << std::endl;
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
	{
		specularStrength -= 0.01f;
		if (specularStrength < 0.0f)
			specularStrength = 0.0f;
		std::cout << "Especular = " << specularStrength << std::endl;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
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

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

CustomObject load_custom_object(std::pair<std::string, std::string> fileNameAndTexture)
{
	CustomObject customObject;

	std::vector<float> vector = read_csv_file(fileNameAndTexture.first);
	std::string textureFileName = fileNameAndTexture.second;

	int vectorSize = vector.size();
	float* vertices = new float[vectorSize];
	for (size_t i = 0; i < vectorSize; i++)
	{
		vertices[i] = vector[i];
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	unsigned int VBO, objVAO;
	glGenVertexArrays(1, &objVAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(objVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vectorSize, vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTICE_DEFINITION * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTICE_DEFINITION * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTICE_DEFINITION * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, VERTICE_DEFINITION * sizeof(float), (void*)(9 * sizeof(float)));
	glEnableVertexAttribArray(3);

	if (textureFileName != "")
	{
		customObject.texture = load_object_texture(textureFileName);
		customObject.drawTexture = true;
	}
	else
		customObject.drawTexture = false;

	customObject.points = vectorSize / VERTICE_DEFINITION;
	customObject.VAO = objVAO;
	customObject.VBO = VBO;

	return customObject;
}

unsigned int load_object_texture(std::string textureFileName)
{
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	unsigned char* data = stbi_load(textureFileName.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		// PNG with transparent background
		std::size_t isPng = textureFileName.find(".png");
		if (isPng != std::string::npos)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	return texture;
}