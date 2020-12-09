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
	unsigned int vao;
	unsigned int vbo;
	unsigned int texture;
	int points;
	bool draw_texture;
} custom_object;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void process_input(GLFWwindow* window);
custom_object load_custom_object(const std::pair<std::string, std::string>& file_name_and_texture);
unsigned int load_object_texture(const std::string& texture_file_name);

// settings
const unsigned int scr_width = 800;
const unsigned int scr_height = 600;
const unsigned int vertice_definition = 11; //3 Positions + 3 Colors + 3 Normal Vector + 2 Texture Coordinates

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 6.0f));
float last_x = scr_width / 2.0f;
float last_y = scr_height / 2.0f;
bool first_mouse = true;

// timing
float delta_time = 0.0f;
float last_frame = 0.0f;

// lighting
glm::vec3 light_pos(1.2f, 1.0f, 2.0f);

// specular reflex
float specular_strength = 0.5;

int main()
{
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	auto* const window = glfwCreateWindow(scr_width, scr_height, "CSV Renderer", nullptr, nullptr);
	if (window == 0)
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
	Shader lighting_shader("src/shaders/phong_lighting.vs", "src/shaders/phong_lighting.fs");

	Shader light_cube_shader("src/shaders/light_cube.vs", "src/shaders/light_cube.fs");

	std::pair<std::string, std::string> modelsAndTextures[] =
	{
		{"src/resources/garden.csv", "src/textures/grass.jpg"},
		{"src/resources/walls.csv", "src/textures/wall.jpg"},
		{"src/resources/door.csv", "src/textures/door.jpg"},
		{"src/resources/window.csv", "src/textures/window.jpg"},
		{"src/resources/ceiling.csv", "src/textures/ceiling.jpg"},
		{"src/resources/rooftop.csv", "src/textures/rooftop.jpg"}
	};

	const int models_and_textures_count = sizeof(modelsAndTextures) / sizeof(modelsAndTextures[0]);
	auto* custom_objects = new custom_object[models_and_textures_count];
	for (auto i = 0; i < models_and_textures_count; i++)
		custom_objects[i] = load_custom_object(modelsAndTextures[i]);

	auto sun = load_custom_object({ "src/resources/sun.csv", "" });

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		const float current_frame = glfwGetTime();
		delta_time = current_frame - last_frame;
		last_frame = current_frame;

		// input
		process_input(window);

		// render
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// change the light's position values over time (can be done anywhere in the render loop actually, but try to do it at least before using the light source positions)
		light_pos.x = sin(glfwGetTime()) * 2.0f;
		light_pos.y = sin(glfwGetTime()) * 2.0f;
		light_pos.z = cos(glfwGetTime()) * 2.0f;

		// be sure to activate shader when setting uniforms/drawing objects
		lighting_shader.use();
		lighting_shader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
		lighting_shader.setVec3("lightPos", light_pos);
		lighting_shader.setVec3("viewPos", camera.Position);
		lighting_shader.setFloat("specularStrength", specular_strength);

		// view/projection transformations
		auto projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(scr_width) / static_cast<float>(scr_height), 0.1f, 100.0f);
		auto view = camera.GetViewMatrix();
		lighting_shader.setMat4("projection", projection);
		lighting_shader.setMat4("view", view);

		// world transformation
		auto model = glm::mat4(1.0f);
		lighting_shader.setMat4("model", model);

		// render objects
		for (auto i = 0; i < models_and_textures_count; i++)
		{
			glBindTexture(GL_TEXTURE_2D, custom_objects[i].texture);
			lighting_shader.setBool("drawTexture", custom_objects[i].draw_texture);
			glBindVertexArray(custom_objects[i].vao);
			glDrawArrays(GL_TRIANGLES, 0, custom_objects[i].points);
		}

		// also draw the lamp object
		light_cube_shader.use();
		light_cube_shader.setMat4("projection", projection);
		light_cube_shader.setMat4("view", view);
		model = glm::mat4(1.0f);
		model = translate(model, light_pos);
		model = scale(model, glm::vec3(0.2f)); // a smaller cube
		light_cube_shader.setMat4("model", model);

		glBindVertexArray(sun.vao);
		glDrawArrays(GL_TRIANGLES, 0, sun.points);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	for (auto i = 0; i < models_and_textures_count; i++)
	{
		glDeleteVertexArrays(1, &custom_objects[i].vao);
		glDeleteBuffers(1, &custom_objects[i].vbo);
	}

	glDeleteVertexArrays(1, &sun.vao);
	glDeleteBuffers(1, &sun.vbo);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void process_input(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, delta_time);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, delta_time);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, delta_time);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, delta_time);
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
	{
		specular_strength += 0.01f;
		if (specular_strength > 5.0f)
			specular_strength = 5.0f;
		std::cout << "Especular = " << specular_strength << std::endl;
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
	{
		specular_strength -= 0.01f;
		if (specular_strength < 0.0f)
			specular_strength = 0.0f;
		std::cout << "Especular = " << specular_strength << std::endl;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, const int width, const int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, const double xpos, const double ypos)
{
	if (first_mouse)
	{
		last_x = xpos;
		last_y = ypos;
		first_mouse = false;
	}

	const float xoffset = xpos - last_x;
	const float yoffset = last_y - ypos; // reversed since y-coordinates go from bottom to top

	last_x = xpos;
	last_y = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow*, double, const double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

custom_object load_custom_object(const std::pair<std::string, std::string>& file_name_and_texture)
{
	custom_object custom_object;

	auto vector = read_csv_file(file_name_and_texture.first);
	const auto texture_file_name = file_name_and_texture.second;

	const int vector_size = vector.size();
	auto* const vertices = new float[vector_size];
	for (size_t i = 0; i < vector_size; i++)
		vertices[i] = vector[i];

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	unsigned int vbo, obj_vao;
	glGenVertexArrays(1, &obj_vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(obj_vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vector_size, vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertice_definition * sizeof(float), static_cast<void*>(0));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertice_definition * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertice_definition * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, vertice_definition * sizeof(float), reinterpret_cast<void*>(9 * sizeof(float)));
	glEnableVertexAttribArray(3);

	if (!texture_file_name.empty())
	{
		custom_object.texture = load_object_texture(texture_file_name);
		custom_object.draw_texture = true;
	}
	else
		custom_object.draw_texture = false;

	custom_object.points = vector_size / vertice_definition;
	custom_object.vao = obj_vao;
	custom_object.vbo = vbo;

	return custom_object;
}

unsigned int load_object_texture(const std::string& texture_file_name)
{
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nr_channels;
	stbi_set_flip_vertically_on_load(1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	auto* const data = stbi_load(texture_file_name.c_str(), &width, &height, &nr_channels, 0);
	if (data)
	{
		// PNG with transparent background
		const auto is_png = texture_file_name.find(".png");
		if (is_png != std::string::npos)
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