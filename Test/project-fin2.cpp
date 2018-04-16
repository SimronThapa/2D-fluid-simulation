#include "stdafx.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <random>
// GLEW
#include <GL/glew.h>
// GLFW
#include <GLFW/glfw3.h>
// Other Libs
#include <SOIL.h>
// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace std;

// Other includes
#include "Shader.h"

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
void copyTex(GLuint fromTexture, GLuint toTexture, GLuint VAO);
void copyTexTo(GLuint toTexture, GLuint VAO);
void drawOn(GLuint VAO);
void swap(GLuint* texture1, GLuint* texture2);

// Window dimensions
const GLuint WIDTH = 600, HEIGHT = 600;

const int JACOBI_ITERATIONS = 10;
const double PI = 3.14159265335987;
const float deltaT = 1 / 120.0f;
const float density = 1.0f;
const float epsilonX = 1 / WIDTH;
const float epsilonY = 1 / HEIGHT;
double dyeX = -1.0, deltaX = -1.0;
double dyeY = -1.0, deltaY = -1.0;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct mesh {
	GLfloat vertX, vertY;
	GLfloat posX, posY;
} arrowMesh[1200];

// The MAIN function, from here we start the application and run the game loop
int main() {
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Drawing", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, WIDTH, HEIGHT);
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// Build and compile our shader program
	Shader screenShader("screenVertex.shader", "screenFragment.shader");
	Shader initColorFuncShader("screenVertex.shader", "initColorFuncFragment.shader");
	Shader initVelFuncShader("screenVertex.shader", "initVelFuncFragment.shader");
	Shader initPressureFuncShader("screenVertex.shader", "initPressureFuncFragment.shader");
	Shader advectShader("screenVertex.shader", "advectFragment.shader");
	Shader addSplatShader("screenVertex.shader", "addSplatFragment.shader");
	Shader divergenceShader("screenVertex.shader", "divergenceFragment.shader");
	Shader jacobiIterationShader("screenVertex.shader", "jacobiIterationFragment.shader");
	Shader subPressureGradientShader("screenVertex.shader", "subPressureGradientFragment.shader");
	Shader arrowShader("drawArrowVertex.shader", "drawArrowFragment.shader");

	screenShader.Use();
	GLfloat initTime = glfwGetTime();

	// Framebuffer (regroups 0, 1, or more textures, and 0 or 1 depth buffer)
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Generate texture
	GLuint texColorBuffer;
	glGenTextures(1, &texColorBuffer);
	glBindTexture(GL_TEXTURE_2D, texColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Attach it to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

	// Render buffer object (for depth)
	GLuint rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	GLuint color0;
	glGenTextures(1, &color0);
	glBindTexture(GL_TEXTURE_2D, color0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, color0, 0);

	GLuint color1;
	glGenTextures(1, &color1);
	glBindTexture(GL_TEXTURE_2D, color1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, color1, 0);

	GLuint velocity0;
	glGenTextures(1, &velocity0);
	glBindTexture(GL_TEXTURE_2D, velocity0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, velocity0, 0);

	GLuint velocity1;
	glGenTextures(1, &velocity1);
	glBindTexture(GL_TEXTURE_2D, velocity1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, velocity1, 0);

	GLuint divergence;
	glGenTextures(1, &divergence);
	glBindTexture(GL_TEXTURE_2D, divergence);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, divergence, 0);

	GLuint pressure0;
	glGenTextures(1, &pressure0);
	glBindTexture(GL_TEXTURE_2D, pressure0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, GL_TEXTURE_2D, pressure0, 0);

	GLuint pressure1;
	glGenTextures(1, &pressure1);
	glBindTexture(GL_TEXTURE_2D, pressure1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT7, GL_TEXTURE_2D, pressure1, 0);

	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	GLenum readBuffers = GL_COLOR_ATTACHMENT1;
	glReadBuffer(readBuffers);

	//GLenum drawBuffers[8] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
	//							GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
	//glDrawBuffers(8, drawBuffers);

	// Check that framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Quad vertices to fill screen texture
	GLfloat quadVertices[] = {
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};

	// Screen quad VAO
	GLuint quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// Tex coordinate attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Arrow vertices
	glm::vec2 arrowVertices[] = {
		glm::vec2(0, 0.2),
		glm::vec2(1, 0),
		glm::vec2(0, -0.2)
	};

	int INTERVAL = 30;
	int l = 0;

	for (int i = INTERVAL / 2; i < HEIGHT; i += INTERVAL) {
		for (int j = INTERVAL / 2; j < WIDTH; j += INTERVAL) {
			for (int k = 0; k < 3; ++k) {
				arrowMesh[l].vertX = arrowVertices[k].x;
				arrowMesh[l].vertY = arrowVertices[k].y;

				arrowMesh[l].posX = 2 * j / WIDTH - 1;
				arrowMesh[l].posY = 2 * i / HEIGHT - 1;
				++l;
			}
		}
	}

	// Arrow VAO
	GLuint arrVAO, arrVBO;
	glGenVertexArrays(1, &arrVAO);
	glGenBuffers(1, &arrVBO);
	glBindVertexArray(arrVAO);
	glBindBuffer(GL_ARRAY_BUFFER, arrVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arrowMesh), &arrowMesh, GL_STATIC_DRAW);

	// Vertex attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// Position attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	initColorFuncShader.Use();
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	copyTex(texColorBuffer, color0, quadVAO);
	//copyTexTo(color0, quadVAO);

	initVelFuncShader.Use();
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	//copyTexTo(velocity0, quadVAO);
	copyTex(texColorBuffer, velocity0, quadVAO);

	initPressureFuncShader.Use();
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	//copyTexTo(pressure0, quadVAO);
	copyTex(texColorBuffer, pressure0, quadVAO);

	initColorFuncShader.Use();
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	copyTex(texColorBuffer, color1, quadVAO);
	//copyTexTo(color0, quadVAO);

	initVelFuncShader.Use();
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	//copyTexTo(velocity0, quadVAO);
	copyTex(texColorBuffer, velocity1, quadVAO);

	initPressureFuncShader.Use();
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	//copyTexTo(pressure0, quadVAO);
	copyTex(texColorBuffer, pressure1, quadVAO);

	GLuint temp;

	// Game loop
	while (!glfwWindowShouldClose(window)) {
		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		// Per-frame time logic
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Mouse position
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		// Render
		// Bind to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Advection, result in velocity0
		advectShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, velocity0);

		glUniform1f(glGetUniformLocation(advectShader.Program, "deltaT"), deltaT);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		copyTex(texColorBuffer, velocity1, quadVAO);
		temp = velocity0;
		velocity0 = velocity1;
		velocity1 = temp;

		// Calculate divergence, result in divergence
		divergenceShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glUniform1f(glGetUniformLocation(divergenceShader.Program, "deltaT"), deltaT);
		glUniform1f(glGetUniformLocation(divergenceShader.Program, "rho"), density);
		glUniform1f(glGetUniformLocation(divergenceShader.Program, "epsilonX"), epsilonX);
		glUniform1f(glGetUniformLocation(divergenceShader.Program, "epsilonY"), epsilonY);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		copyTex(texColorBuffer, divergence, quadVAO);

		// Calculate pressure, result in pressure0
		for (int i = 0; i < JACOBI_ITERATIONS; ++i) {
			jacobiIterationShader.Use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, divergence);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, pressure0);
			glUniform1f(glGetUniformLocation(jacobiIterationShader.Program, "epsilonX"), epsilonX);
			glUniform1f(glGetUniformLocation(jacobiIterationShader.Program, "epsilonY"), epsilonY);

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);

			copyTex(texColorBuffer, pressure1, quadVAO);
			temp = pressure0;
			pressure0 = pressure1;
			pressure1 = temp;
		}

		// Subtract pressure gradient from advected velocity, result in velocity0
		subPressureGradientShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pressure0);
		glUniform1f(glGetUniformLocation(subPressureGradientShader.Program, "epsilonX"), epsilonX);
		glUniform1f(glGetUniformLocation(subPressureGradientShader.Program, "epsilonY"), epsilonY);
		glUniform1f(glGetUniformLocation(subPressureGradientShader.Program, "deltaT"), deltaT);
		glUniform1f(glGetUniformLocation(subPressureGradientShader.Program, "rho"), density);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		copyTex(texColorBuffer, velocity1, quadVAO);
		/*temp = velocity0;
		velocity0 = velocity1;
		velocity1 = temp;*/

		// Advect color field, result in color0
		advectShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, color0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, velocity1);
		glUniform1f(glGetUniformLocation(advectShader.Program, "deltaT"), deltaT);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		copyTex(texColorBuffer, color1, quadVAO);
		temp = color0;
		color0 = color1;
		color1 = temp;

		// Dye spots
		addSplatShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, color0);
		glUniform1f(glGetUniformLocation(addSplatShader.Program, "radius"), 0.01);
		glUniform4f(glGetUniformLocation(addSplatShader.Program, "change"), 0.004, -0.002, -0.002, 0.0);
		glUniform2f(glGetUniformLocation(addSplatShader.Program, "center"), 0.2, 0.2);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		copyTex(texColorBuffer, color1, quadVAO);
		temp = color0;
		color0 = color1;
		color1 = temp;

		// Change velocity from mouse
		addSplatShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glUniform1f(glGetUniformLocation(addSplatShader.Program, "radius"), 0.01);
		glUniform4f(glGetUniformLocation(addSplatShader.Program, "change"), 10.0 * deltaX / WIDTH, -10.0 * deltaY / HEIGHT, 0.0, 0.0);
		glUniform2f(glGetUniformLocation(addSplatShader.Program, "center"), dyeX, dyeY);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		copyTex(texColorBuffer, velocity1, quadVAO);
		temp = velocity0;
		velocity0 = velocity1;
		velocity1 = temp;

		glClear(GL_DEPTH_BUFFER_BIT);

		// Bind to default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, WIDTH, HEIGHT);
		glDisable(GL_DEPTH_TEST);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		screenShader.Use();
		glBindVertexArray(quadVAO);
		glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, velocity0);
		glBindTexture(GL_TEXTURE_2D, texColorBuffer); // use the color attachment texture as the texture of the quad plane
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindTexture(GL_TEXTURE_2D, 0);

		arrowShader.Use();
		glBindVertexArray(arrVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glDrawArrays(GL_TRIANGLES, 0, 1200);
		glBindTexture(GL_TEXTURE_2D, 0);

		//glUniform2f(glGetUniformLocation(screenShader.Program, "mousePos"), (xpos * 2) / WIDTH - 1, -(ypos * 2) / HEIGHT + 1);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &quadVAO);
	glDeleteFramebuffers(1, &fbo);

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

void copyTexTo(GLuint toTexture, GLuint VAO) {
	GLuint tempFBO, tempTex;

	// Generate texture
	glGenTextures(1, &tempTex);
	glBindTexture(GL_TEXTURE_2D, tempTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	// Generate framebuffer
	glGenFramebuffers(1, &tempFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempTex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Update texture
	glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, WIDTH, HEIGHT);

	// Draw on framebuffer
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Copy texture
	glBindFramebuffer(GL_READ_FRAMEBUFFER, tempFBO);
	glBindTexture(GL_TEXTURE_2D, toTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, WIDTH, HEIGHT);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glPopAttrib();
}

void copyTex(GLuint fromTexture, GLuint toTexture, GLuint VAO) {
	GLuint tempFBO;

	// Generate framebuffer
	glGenFramebuffers(1, &tempFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fromTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Update texture
	glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, WIDTH, HEIGHT);

	// Draw on framebuffer
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Copy texture
	glBindFramebuffer(GL_READ_FRAMEBUFFER, tempFBO);
	glBindTexture(GL_TEXTURE_2D, toTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, WIDTH, HEIGHT);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glPopAttrib();
}

void drawOn(GLuint VAO) {
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void swap(GLuint* texture1, GLuint* texture2) {
	GLuint* temp;
	temp = texture2;
	texture2 = texture1;
	texture1 = temp;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

bool drag = false;
double prevxpos, prevypos;

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS && !drag) {
			glfwGetCursorPos(window, &prevxpos, &prevypos);
			drag = true;
		}
		else if (action == GLFW_RELEASE) {
			drag = false;
		}
	}
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
	if (drag) {
		deltaX = xpos - prevxpos;
		deltaY = ypos - prevypos;
		dyeX = xpos / WIDTH;
		dyeY = 1 - ypos / HEIGHT;
		prevxpos = xpos; prevypos = ypos;
	}
}