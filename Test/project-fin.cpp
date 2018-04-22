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
// OpenCV
#include <opencv2/imgproc/imgproc.hpp> 
#include <opencv2/core/core.hpp>      
#include <opencv2/highgui/highgui.hpp>
using namespace std;
using namespace cv;

// Other includes
#include "Shader.h"

// Nuklear
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL2_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_glfw_gl2.h"

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);

// Window dimensions
const GLuint WIDTH = 600, HEIGHT = 600, SET_WIDTH = 300, SET_HEIGHT = 570;

const int JACOBI_ITERATIONS = 10;
const double PI = 3.14159265335987;
const float density = 1.0f;
const float epsilonX = 1 / (float)WIDTH;
const float epsilonY = 1 / (float)HEIGHT;
double dyeX = -1.0, deltaX = -1.0;
double dyeY = -1.0, deltaY = -1.0;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool drag = false, velChange = true, colChange = false;
float deltaT = 1.0 / 120.0f;
int velID = 0, prevVelID = 0, colID = 0, prevColID = 0, mouseID = 0;
float dyeRadius = 0.01;
struct nk_colorf dyeColor;

double area_limit = 700;
Scalar lowerBound = Scalar(50, 100, 100);  // green
Scalar upperBound = Scalar(120, 255, 255);
int posX = 0;
int posY = 0;

// The MAIN function, from here we start the application and run the game loop
int main() {
	// Init GLFW
	glfwInit();

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "2D Fluid Simulation", NULL, NULL);
	GLFWwindow* window2 = glfwCreateWindow(SET_WIDTH, SET_HEIGHT, "Settings", NULL, window);
	int wid = 0, hei = 0;
	glfwGetWindowSize(window2, &wid, &hei);
	glfwMakeContextCurrent(window);

	struct nk_context *ctx;
	ctx = nk_glfw3_init(window2, NK_GLFW3_INSTALL_CALLBACKS);
	{ struct nk_font_atlas *atlas;
	nk_glfw3_font_stash_begin(&atlas);
	nk_glfw3_font_stash_end(); }

	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 24);

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
	glDisable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);
	glEnable(GL_MULTISAMPLE);

	// Build and compile our shader program
	Shader screenShader("screenVertex.shader", "screenFragment.shader");
	Shader initColorFuncShader("screenVertex.shader", "initColorFuncFragment.shader");
	Shader initVelFuncShader("screenVertex.shader", "initVelFuncFragment.shader");
	Shader initPressureFuncShader("screenVertex.shader", "initPressureFuncFragment.shader");
	Shader advectShader("screenVertex.shader", "advectFragment.shader");
	Shader advectColShader("screenVertex.shader", "advectColFragment.shader");
	Shader addSplatShader("screenVertex.shader", "addSplatFragment.shader");
	Shader addSplatColShader("screenVertex.shader", "addSplatColFragment.shader");
	Shader divergenceShader("screenVertex.shader", "divergenceFragment.shader");
	Shader jacobiIterationShader("screenVertex.shader", "jacobiIterationFragment.shader");
	Shader subPressureGradientShader("screenVertex.shader", "subPressureGradientFragment.shader");
	Shader arrowShader("drawArrowVertex.shader", "drawArrowFragment.shader");
	Shader texVelCopyShader("screenVertex.shader", "texVelCopyFragment.shader");
	Shader texColCopyShader("screenVertex.shader", "texColCopyFragment.shader");
	Shader texPresCopyShader("screenVertex.shader", "texPresCopyFragment.shader");

	glLinkProgram(screenShader.Program);
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
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT7, GL_TEXTURE_2D, texColorBuffer, 0);

	// The depth renderbuffer
	GLuint depthbuffer;
	glGenTextures(1, &depthbuffer);
	glBindTexture(GL_TEXTURE_2D, depthbuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, WIDTH, HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthbuffer, 0);


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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color0, 0);

	GLuint color1;
	glGenTextures(1, &color1);
	glBindTexture(GL_TEXTURE_2D, color1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, color1, 0);

	GLuint velocity0;
	glGenTextures(1, &velocity0);
	glBindTexture(GL_TEXTURE_2D, velocity0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, velocity0, 0);

	GLuint velocity1;
	glGenTextures(1, &velocity1);
	glBindTexture(GL_TEXTURE_2D, velocity1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, velocity1, 0);

	GLuint divergence;
	glGenTextures(1, &divergence);
	glBindTexture(GL_TEXTURE_2D, divergence);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, divergence, 0);

	GLuint pressure0;
	glGenTextures(1, &pressure0);
	glBindTexture(GL_TEXTURE_2D, pressure0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, pressure0, 0);

	GLuint pressure1;
	glGenTextures(1, &pressure1);
	glBindTexture(GL_TEXTURE_2D, pressure1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, GL_TEXTURE_2D, pressure1, 0);

	GLenum drawBuffers[7] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6 };
	glDrawBuffers(7, drawBuffers);

	//GLenum readBuffers = GL_COLOR_ATTACHMENT0;
	//glReadBuffer(readBuffers);

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
	int ARROW_SIZE = 0;
	GLfloat arrowMesh[100000];

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
				arrowMesh[l] = arrowVertices[k].x;
				arrowMesh[++l] = arrowVertices[k].y;

				arrowMesh[++l] = 2 * j / (float)WIDTH - 1;
				arrowMesh[++l] = 2 * i / (float)HEIGHT - 1;
				++l;
			}
		}
	}
	ARROW_SIZE = l;

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

	VideoCapture cap(0);
	Mat frame;
	cap >> frame;

	bool initVel = true, initCol = true;
	bool showArrow = false;
	dyeColor.r = 0.1; dyeColor.g = 0.1; dyeColor.b = 0.5;

	// Game loop
	while (!glfwWindowShouldClose(window)) {
		// OpenCV camera
		cap >> frame;
		flip(frame, frame, 1);

		// Median filter to decrease the background noise
		medianBlur(frame, frame, 5);

		// Get thresholded image
		Mat imgHSV = frame.clone();
		cvtColor(frame, imgHSV, CV_BGR2HSV);	// convert to HSV
		Mat imgThresh = Mat::zeros(frame.rows, frame.cols, CV_8UC1);
		inRange(imgHSV, lowerBound, upperBound, imgThresh);

		// Morphological opening (remove small objects from background)
		erode(imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		dilate(imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

		// Calculate the moments to estimate the position of the object
		Moments moment = moments(imgThresh);

		// Actual moment values
		double moment10 = moment.m10;	//extract spatial moment 10
		double moment01 = moment.m01;	//extract spatial moment 01
		double area = moment.m00;		//extract central moment 00

		int lastX = posX;
		int lastY = posY;
		posX = 0;
		posY = 0;

		if (moment10 / area >= 0 && moment10 / area < 1280 && moment01 / area >= 0
			&& moment01 / area < 1280 && area > area_limit) {
			posX = moment10 / area;
			posY = moment01 / area;
		}

		// OpenGL window
		glfwMakeContextCurrent(window);

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
		glDisable(GL_DEPTH_TEST);

		if (initVel) {
			initVelFuncShader.Use();
			glUniform1i(glGetUniformLocation(initVelFuncShader.Program, "velID"), velID);
			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);

			initPressureFuncShader.Use();
			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);

			initVel = false;
		}

		if (initCol) {
			initColorFuncShader.Use();
			glUniform1i(glGetUniformLocation(initColorFuncShader.Program, "colID"), colID);
			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);

			initCol = false;
		}

		// Advection, result in velocity0
		glLinkProgram(advectShader.Program);
		advectShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glUniform1i(glGetUniformLocation(advectShader.Program, "inputTexture"), 0);
		glUniform1i(glGetUniformLocation(advectShader.Program, "velocity"), 1);
		glUniform1f(glGetUniformLocation(advectShader.Program, "deltaT"), deltaT);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		texVelCopyShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity1);
		glUniform1i(glGetUniformLocation(texVelCopyShader.Program, "velocity1"), 0);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Calculate divergence, result in divergence
		glLinkProgram(divergenceShader.Program);
		divergenceShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glUniform1i(glGetUniformLocation(divergenceShader.Program, "velocity"), 0);
		glUniform1f(glGetUniformLocation(divergenceShader.Program, "epsilonX"), epsilonX);
		glUniform1f(glGetUniformLocation(divergenceShader.Program, "epsilonY"), epsilonY);
		glUniform1f(glGetUniformLocation(divergenceShader.Program, "deltaT"), deltaT);
		glUniform1f(glGetUniformLocation(divergenceShader.Program, "rho"), density);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Calculate pressure, result in pressure0
		for (int i = 0; i < JACOBI_ITERATIONS; ++i) {
			glLinkProgram(jacobiIterationShader.Program);
			jacobiIterationShader.Use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, divergence);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, pressure0);
			glUniform1i(glGetUniformLocation(jacobiIterationShader.Program, "divergence"), 0);
			glUniform1i(glGetUniformLocation(jacobiIterationShader.Program, "pressure"), 1);
			glUniform1f(glGetUniformLocation(jacobiIterationShader.Program, "epsilonX"), epsilonX);
			glUniform1f(glGetUniformLocation(jacobiIterationShader.Program, "epsilonY"), epsilonY);

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);

			texPresCopyShader.Use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, pressure1);
			glUniform1i(glGetUniformLocation(texPresCopyShader.Program, "pressure1"), 0);

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Subtract pressure gradient from advected velocity, result in velocity0
		glLinkProgram(subPressureGradientShader.Program);
		subPressureGradientShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pressure0);
		glUniform1i(glGetUniformLocation(subPressureGradientShader.Program, "velocity"), 0);
		glUniform1i(glGetUniformLocation(subPressureGradientShader.Program, "pressure"), 1);
		glUniform1f(glGetUniformLocation(subPressureGradientShader.Program, "epsilonX"), epsilonX);
		glUniform1f(glGetUniformLocation(subPressureGradientShader.Program, "epsilonY"), epsilonY);
		glUniform1f(glGetUniformLocation(subPressureGradientShader.Program, "deltaT"), deltaT);
		glUniform1f(glGetUniformLocation(subPressureGradientShader.Program, "rho"), density);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		texVelCopyShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, velocity1);
		glUniform1i(glGetUniformLocation(texVelCopyShader.Program, "velocity1"), 0);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Advect color field, result in color0
		glLinkProgram(advectColShader.Program);
		advectColShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, color0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, velocity0);
		glUniform1i(glGetUniformLocation(advectColShader.Program, "inputTexture"), 0);
		glUniform1i(glGetUniformLocation(advectColShader.Program, "velocity"), 1);
		glUniform1f(glGetUniformLocation(advectColShader.Program, "deltaT"), deltaT);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		texColCopyShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, color1);
		glUniform1i(glGetUniformLocation(texColCopyShader.Program, "color1"), 0);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		/*// Dye spots
		glLinkProgram(addSplatColShader.Program);
		addSplatColShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, color0);
		glUniform1i(glGetUniformLocation(addSplatColShader.Program, "inputTex"), 0);
		glUniform1f(glGetUniformLocation(addSplatColShader.Program, "radius"), 0.01);
		glUniform4f(glGetUniformLocation(addSplatColShader.Program, "change"), 0.004, -0.002, -0.002, 0.0);
		glUniform2f(glGetUniformLocation(addSplatColShader.Program, "center"), 0.2, 0.2);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		texColCopyShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, color1);
		glUniform1i(glGetUniformLocation(texColCopyShader.Program, "color1"), 0);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);*/

		// Change velocity from mouse
		if(drag) {
			if (colChange) {
				glLinkProgram(addSplatColShader.Program);
				addSplatColShader.Use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, color0);
				glUniform1i(glGetUniformLocation(addSplatColShader.Program, "inputTex"), 0);
				glUniform1f(glGetUniformLocation(addSplatColShader.Program, "radius"), dyeRadius);
				glUniform4f(glGetUniformLocation(addSplatColShader.Program, "change"), dyeColor.r, dyeColor.g, dyeColor.b, 0.0);
				glUniform2f(glGetUniformLocation(addSplatColShader.Program, "center"), dyeX, dyeY);

				glBindVertexArray(quadVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);
				glBindTexture(GL_TEXTURE_2D, 0);

				texColCopyShader.Use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, color1);
				glUniform1i(glGetUniformLocation(texColCopyShader.Program, "color1"), 0);

				glBindVertexArray(quadVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			
			if (velChange) {
				glLinkProgram(addSplatShader.Program);
				addSplatShader.Use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, velocity0);
				glUniform1i(glGetUniformLocation(addSplatShader.Program, "inputTex"), 0);
				glUniform1f(glGetUniformLocation(addSplatShader.Program, "radius"), dyeRadius);
				glUniform4f(glGetUniformLocation(addSplatShader.Program, "change"), 10.0 * deltaX / WIDTH, -10.0 * deltaY / HEIGHT, 0.0, 0.0);
				glUniform2f(glGetUniformLocation(addSplatShader.Program, "center"), dyeX, dyeY);

				glBindVertexArray(quadVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);
				glBindTexture(GL_TEXTURE_2D, 0);

				texVelCopyShader.Use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, velocity1);
				glUniform1i(glGetUniformLocation(texVelCopyShader.Program, "velocity1"), 0);

				glBindVertexArray(quadVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}

		// Bind to default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, WIDTH, HEIGHT);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		screenShader.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, color0);

		glUniform1i(glGetUniformLocation(screenShader.Program, "screenTexture"), 0);
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (showArrow) {
			arrowShader.Use();
			glBindVertexArray(arrVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, velocity0);
			glUniform1i(glGetUniformLocation(arrowShader.Program, "velocity"), 0);
			glDrawArrays(GL_TRIANGLES, 0, ARROW_SIZE / 4);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glfwMakeContextCurrent(window2);
		
		// Input
		glfwPollEvents();
		nk_glfw3_new_frame();

		// GUI
		if (nk_begin(ctx, "Lighting", nk_rect(0, 0, SET_WIDTH, SET_HEIGHT), NK_WINDOW_SCALABLE)) {
			nk_layout_row_dynamic(ctx, 5, 1);

			/*nk_layout_row_begin(ctx, NK_STATIC, 25, 3); {
				nk_layout_row_push(ctx, 120);
				nk_label(ctx, "Mouse Change", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 80);
				if (nk_option_label(ctx, "Velocity", velChange == true)) velChange = true;
				nk_layout_row_push(ctx, 60);
				if (nk_option_label(ctx, "Colour", velChange == false)) velChange = false;
			}
			nk_layout_row_end(ctx);*/

			static const char *mouseChange[] = { "Velocity", "Colour", "Both" };
			nk_layout_row_begin(ctx, NK_STATIC, 25, 2); {
				nk_layout_row_push(ctx, 135);
				nk_label(ctx, "Mouse Change", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 130);
				mouseID = nk_combo(ctx, mouseChange, NK_LEN(mouseChange), mouseID, 25, nk_vec2(200, 200));
			}
			nk_layout_row_end(ctx);

			switch (mouseID) {
				case 0: velChange = true; colChange = false; break;
				case 1: velChange = false; colChange = true; break;
				case 2: velChange = true; colChange = true; break;
			}

			nk_layout_row_dynamic(ctx, 10, 1);

			nk_layout_row_begin(ctx, NK_STATIC, 25, 2); {
				nk_layout_row_push(ctx, 105);
				nk_label(ctx, "Mouse Radius", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 165);
				nk_slider_float(ctx, 0.001, &dyeRadius, 0.02, 0.001);
			}
			nk_layout_row_end(ctx);

			nk_layout_row_dynamic(ctx, 10, 1);

			nk_layout_row_dynamic(ctx, 25, 1);
			nk_label(ctx, "Mouse Dye Colour", NK_TEXT_LEFT);
			nk_layout_row_dynamic(ctx, 120, 1);
			dyeColor = nk_color_picker(ctx, dyeColor, NK_RGB);
			nk_layout_row_dynamic(ctx, 25, 1);
			dyeColor.r = nk_propertyf(ctx, "#R:", 0, dyeColor.r, 1.0f, 0.01f, 0.005f);
			dyeColor.g = nk_propertyf(ctx, "#G:", 0, dyeColor.g, 1.0f, 0.01f, 0.005f);
			dyeColor.b = nk_propertyf(ctx, "#B:", 0, dyeColor.b, 1.0f, 0.01f, 0.005f);

			nk_layout_row_dynamic(ctx, 10, 1);

			nk_layout_row_begin(ctx, NK_STATIC, 25, 3); {
				nk_layout_row_push(ctx, 120);
				nk_label(ctx, "Velocity Arrows", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 80);
				if (nk_option_label(ctx, "Hide", showArrow == false)) showArrow = false;
				nk_layout_row_push(ctx, 60);
				if (nk_option_label(ctx, "Show", showArrow == true)) showArrow = true;
			}
			nk_layout_row_end(ctx);

			nk_layout_row_dynamic(ctx, 10, 1);

			static const char *velTypes[] = { "Whirlpool", "Linear", "None" };
			nk_layout_row_begin(ctx, NK_STATIC, 25, 2); {
				nk_layout_row_push(ctx, 135);
				nk_label(ctx, "Velocity Function", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 130);
				velID = nk_combo(ctx, velTypes, NK_LEN(velTypes), velID, 25, nk_vec2(200, 200));
				if (prevVelID != velID) {
					prevVelID = velID;
					initVel = true;
				}
			}

			nk_layout_row_dynamic(ctx, 10, 1);

			static const char *colTypes[] = { "None", "Tiled" };
			nk_layout_row_begin(ctx, NK_STATIC, 25, 2); {
				nk_layout_row_push(ctx, 135);
				nk_label(ctx, "Colour Function", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 130);
				colID = nk_combo(ctx, colTypes, NK_LEN(colTypes), colID, 25, nk_vec2(200, 200));
				if (prevColID != colID) {
					prevColID = colID;
					initCol = true;
				}
			}

			nk_layout_row_dynamic(ctx, 10, 1);

			nk_layout_row_begin(ctx, NK_STATIC, 25, 2); {
				nk_layout_row_push(ctx, 105);
				nk_label(ctx, "Simulation Rate", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 165);
				nk_slider_float(ctx, 0.005, &deltaT, 0.02, 0.001);
			}
			nk_layout_row_end(ctx);

			nk_layout_row_dynamic(ctx, 15, 1);

			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_button_label(ctx, "Reset")) {
				initVel = true;
				initCol = true;
			}
		}
		nk_end(ctx);

		// Draw
		glfwGetWindowSize(window2, &wid, &hei);
		glViewport(0, 0, wid, hei);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		nk_glfw3_render(NK_ANTI_ALIASING_ON);

		// Swap the screen buffers
		glfwSwapBuffers(window);
		glfwSwapBuffers(window2);
	}

	glDeleteVertexArrays(1, &quadVAO);
	glDeleteFramebuffers(1, &fbo);

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

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