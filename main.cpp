/*
Type your name and student ID here
	- Name: Tsoi Chak Yu
	- Student ID: 1155126232
*/

#include "Dependencies/glew/glew.h"
#include "Dependencies/GLFW/glfw3.h"

#include "Dependencies/glm/glm.hpp"
#include "Dependencies/glm/gtc/matrix_transform.hpp"
#include "Dependencies/glm/gtc/type_ptr.hpp"
#include<math.h>
#include <iostream>
#include <fstream>

#include "Camera.h"
#include "Shader.h"
#include "Model.h"

void mouse_callback(GLFWwindow* window, double xpos, double ypos);


GLint programID; GLuint vao1, vao2, vao3, vao4, index1, index2, index3,index4,skyBoxVAO,skyBoxVBO, skyBoxCubeMapTextureID;
GLint skyBoxProgramID;
int rotate = 0, move_x = 0, move_z = 0, scale_g=0;
float D_x = 1.0f, D_y = 1.0f, D_z = 1.0f, D_r = 3.0f;//Delta of x,y,z,rotation respectively
glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
bool Mouse = true;
float yaw = -90.0f;	
float pitch = 0.0f;
float x_last = 800.0f / 2.0;
float y_last = 600.0 / 2.0;
float fov = 45.0f;

glm::vec3 cameraLocalPosition = glm::vec3(5, 5, 5);
glm::vec4 cameraPointPosition;
glm::vec4 cameraWorldPosition;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (Mouse)
	{
		x_last = xpos;
		y_last = ypos;
		Mouse = false;
	}

	float xoffset = xpos - x_last;
	float yoffset = y_last - ypos; // reversed since y-coordinates go from bottom to top
	x_last = xpos;
	y_last = ypos;

	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraPos = glm::normalize(front);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (fov >= 1.0f && fov <= 45.0f)
		fov -= yoffset;
	if (fov <= 1.0f)
		fov = 1.0f;
	if (fov >= 45.0f)
		fov = 45.0f;
}
void get_OpenGL_info() {
	// OpenGL information
	const GLubyte* name = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* glversion = glGetString(GL_VERSION);
	std::cout << "OpenGL company: " << name << std::endl;
	std::cout << "Renderer name: " << renderer << std::endl;
	std::cout << "OpenGL version: " << glversion << std::endl;
}

bool checkStatus(
	GLuint objectID,
	PFNGLGETSHADERIVPROC objectPropertyGetterFunc,
	PFNGLGETSHADERINFOLOGPROC getInfoLogFunc,
	GLenum statusType)
{
	GLint status;
	objectPropertyGetterFunc(objectID, statusType, &status);
	if (status != GL_TRUE)
	{
		GLint infoLogLength;
		objectPropertyGetterFunc(objectID, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar* buffer = new GLchar[infoLogLength];

		GLsizei bufferSize;
		getInfoLogFunc(objectID, infoLogLength, &bufferSize, buffer);
		std::cout << buffer << std::endl;

		delete[] buffer;
		return false;
	}
	return true;
}

bool checkShaderStatus(GLuint shaderID) {
	return checkStatus(shaderID, glGetShaderiv, glGetShaderInfoLog, GL_COMPILE_STATUS);
}

bool checkProgramStatus(GLuint programID) {
	return checkStatus(programID, glGetProgramiv, glGetProgramInfoLog, GL_LINK_STATUS);
}

std::string readShaderCode(const char* fileName) {
	std::ifstream meInput(fileName);
	if (!meInput.good()) {
		std::cout << "File failed to load ... " << fileName << std::endl;
		exit(1);
	}
	return std::string(
		std::istreambuf_iterator<char>(meInput),
		std::istreambuf_iterator<char>()
	);
}

int installShaders(const char* vertexShader, const char* fragmentShader)
{
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	const GLchar* adapter[1];
	string temp = readShaderCode(vertexShader);
	adapter[0] = temp.c_str();
	glShaderSource(vertexShaderID, 1, adapter, 0);
	temp = readShaderCode(fragmentShader);
	adapter[0] = temp.c_str();
	glShaderSource(fragmentShaderID, 1, adapter, 0);

	glCompileShader(vertexShaderID);
	glCompileShader(fragmentShaderID);

	if (!checkShaderStatus(vertexShaderID) || !checkShaderStatus(fragmentShaderID)) {
		printf("Cannot create program with %s, %s\n", vertexShader, fragmentShader);
		return -1;
	}

	int program = glCreateProgram();
	glAttachShader(program, vertexShaderID);
	glAttachShader(program, fragmentShaderID);
	glLinkProgram(program);

	if (!checkProgramStatus(program)) {
		printf("Cannot create program with %s, %s\n", vertexShader, fragmentShader);
		return -1;
	}

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	return program;
}



GLuint loadCubemap(vector<const GLchar*> faces) {
	unsigned int width, height;
	const GLchar* imagepath;
	GLuint textureID;
	glGenTextures(1, &textureID);
	glActiveTexture(GL_TEXTURE5); //we use texture 5 for skybox
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	for (GLuint i = 0; i < faces.size(); i++) {
		imagepath = faces.at(i);
		printf("Reading image %s\n", imagepath);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int imageSize;
		unsigned char* data;
		#pragma warning(suppress : 4996)
		FILE* file = fopen(imagepath, "rb");
		if (!file) { printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar(); return 0; }

		if (fread(header, 1, 54, file) != 54) {
			printf("Not a correct BMP file\n");
			return 0;
		}
		if (header[0] != 'B' || header[1] != 'M') {
			printf("Not a correct BMP file\n");
			return 0;
		}
		if (*(int*)&(header[0x1E]) != 0) { printf("Not a correct BMP file\n");    return 0; }
		if (*(int*)&(header[0x1C]) != 24) { printf("Not a correct BMP file\n");    return 0; }

		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize == 0) imageSize = width * height * 3;
		if (dataPos == 0) dataPos = 54;

		data = new unsigned char[imageSize];
		fread(data, 1, imageSize, file);
		fclose(file);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		delete[] data;
		printf("Sussess\n");
	}


	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	return textureID;
}

void sendSkyboxData() {
	float skyBoxVertices[] = {
		// positions          
		-1.0f, +1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		+1.0f, -1.0f, -1.0f,
		+1.0f, -1.0f, -1.0f,
		+1.0f, +1.0f, -1.0f,
		-1.0f, +1.0f, -1.0f,

		-1.0f, -1.0f, +1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, +1.0f, -1.0f,
		-1.0f, +1.0f, -1.0f,
		-1.0f, +1.0f, +1.0f,
		-1.0f, -1.0f, +1.0f,

		+1.0f, -1.0f, -1.0f,
		+1.0f, -1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, -1.0f,
		+1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f, +1.0f,
		-1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		+1.0f, -1.0f, +1.0f,
		-1.0f, -1.0f, +1.0f,

		-1.0f, +1.0f, -1.0f,
		+1.0f, +1.0f, -1.0f,
		+1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		-1.0f, +1.0f, +1.0f,
		-1.0f, +1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, +1.0f,
		+1.0f, -1.0f, -1.0f,
		+1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, +1.0f,
		+1.0f, -1.0f, +1.0f
	};

	glGenVertexArrays(1, &skyBoxVAO);
	glGenBuffers(1, &skyBoxVBO);
	glBindVertexArray(skyBoxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyBoxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyBoxVertices), &skyBoxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glBindVertexArray(0);

	vector<const GLchar*>earth_faces;

	earth_faces.push_back("object models and textures/texture/universe skybox/right.bmp");
	earth_faces.push_back("object models and textures/texture/universe skybox/left.bmp");
	earth_faces.push_back("object models and textures/texture/universe skybox/top.bmp");
	earth_faces.push_back("object models and textures/texture/universe skybox/bottom.bmp");
	earth_faces.push_back("object models and textures/texture/universe skybox/back.bmp");
	earth_faces.push_back("object models and textures/texture/universe skybox/front.bmp");

	skyBoxCubeMapTextureID = loadCubemap(earth_faces);
}

void sendAlienpeopleData() {
	Model alienpeople("object models and textures/alienpeople.obj");

}
void sendDataToOpenGL() {
	// TODO:
	// create 2D objects and 3D objects and/or lines (points) here and bind to VAOs & VBOs
	sendSkyboxData();
	
}

void paintGL(void) {
	// always run
	// TODO:
	// render your objects and control the transformation here
	glClearColor(+0.0f, 0.0f, 0.0f, 1.0f);  //specify the background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glm:: mat4 ViewMatrix = glm::lookAt(glm::vec3(cameraWorldPosition), glm::vec3(cameraPointPosition), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 ProjectionMatrix = glm::perspective(20.0f, 1.0f, 1.0f, 200.0f);

	glUseProgram(skyBoxProgramID);

	glm::mat4 Skb_ModelMatrix = glm::mat4(1.0f);
	Skb_ModelMatrix = glm::scale(Skb_ModelMatrix, glm::vec3(100.0f));

	glUniformMatrix4fv(glGetUniformLocation(skyBoxProgramID, "M"), 1, GL_FALSE, &Skb_ModelMatrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(skyBoxProgramID, "view"), 1, GL_FALSE, &ViewMatrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(skyBoxProgramID, "projection"), 1, GL_FALSE, &ProjectionMatrix[0][0]);

	glBindVertexArray(skyBoxVAO);
	glUniform1i(glGetUniformLocation(skyBoxProgramID, "skybox"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxCubeMapTextureID);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);	 
	glDepthMask(GL_TRUE);

	glUseProgram(programID);


	// create transformations
	glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 proj = glm::mat4(1.0f);
	
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	
	proj = glm::perspective(glm::radians(fov), 4.0f / 3.0f, 0.1f, 100.0f);

	GLint modelID = glGetUniformLocation(programID, "model");
	GLint viewID = glGetUniformLocation(programID, "view");
	GLint projID = glGetUniformLocation(programID, "proj");

	glUseProgram(skyBoxProgramID);


	glUseProgram(programID);*/

	
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	// TODO:

	float cameraSpeed = 2.5f ;

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	glfwSetScrollCallback(window, scroll_callback);

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (key == GLFW_KEY_I && action == GLFW_PRESS)
		move_z += 1;
	if (key == GLFW_KEY_K && action == GLFW_PRESS)
		move_z -= 1;
	if (key == GLFW_KEY_J && action == GLFW_PRESS)
		move_x += 1;
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
		move_x -= 1;
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		
	if (key == GLFW_KEY_E && action == GLFW_PRESS)
		
	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		scale_g -= 1;
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
		scale_g += 1;

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void initializedGL(void) {
	// run only once
	// TODO:
	programID = installShaders("VertexShaderCode.glsl", "FragmentShaderCode.glsl");
	skyBoxProgramID = installShaders("SkyBoxVertexShaderCode.glsl", "SkyboxFragmentShaderCode.glsl");
	sendDataToOpenGL();
}

int main(int argc, char* argv[]) {
	GLFWwindow* window;

	/* Initialize the glfw */
	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	/* glfw: configure; necessary for MAC */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(800, 600, "Assignment 1", NULL, NULL);
	if (!window) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);

	/* Initialize the glew */
	if (GLEW_OK != glewInit()) {
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}
	get_OpenGL_info();
	initializedGL();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window)) {
		/* Render here */
		paintGL();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
