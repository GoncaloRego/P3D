#pragma comment(lib, "lib/glew32s.lib")
#pragma comment(lib, "lib/glfw3.lib")
#pragma comment(lib, "opengl32.lib")

#pragma region include

using namespace std;
#include <iostream>
#include <vector>

#define GLEW_STATIC
#include <GL\glew.h>


#define GLFW_USE_DWM_SWAP_INTERVAL
#include <GLFW\glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

#include "LoadShaders.hpp"
#include "Camera.hpp"
#include "ObjLoader.hpp"
#include "Vbolndexer.hpp"
#pragma endregion

#define WIDTH 1920
#define HEIGHT 1080

void init(void);
void display(void);
void print_gl_info();
void print_error(int error, const char *description);
void loadTexture(string textureFile);

GLuint programID;

//LIGHT
GLuint programa;

GLuint vertexBuffer;
GLuint uvBuffer;
GLuint normalBuffer;
GLuint elementBuffer;

GLuint vertexArrayID;

GLuint matrixID;
GLuint viewMatrixID;
GLuint modelMatrixID;

GLuint lightID;
GLuint textureID;

//spotLight
GLint lightPosLoc;
GLint lightSpotDirLoc;
GLint lightSpotCutOffLoc;
GLint viewPosLoc;
GLint lightSpotOuterCutOffLoc;

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

bool enableSpotLight = false;

vector< glm::vec3 > vertices;
vector< glm::vec2 > uvs;
vector< glm::vec3 > normals;

vector<unsigned short> indices;
std::vector<glm::vec3> indexed_vertices;
std::vector<glm::vec2> indexed_uvs;
std::vector<glm::vec3> indexed_normals;


GLFWwindow* window;
GLFWmonitor* monitor;

#pragma region Matrices
//Model Matrix//
glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f); //translation
glm::mat4 translationMatrix = glm::translate(glm::mat4(), translation);

float angleInDegrees = 0; //rotation in degrees
glm::vec3 rotationAxis(0, 0, 0); // axis for rotation
glm::mat4 rotationMatrix = glm::rotate(angleInDegrees, rotationAxis);

float scale = 2; //scale
glm::mat4 scaleMatrix = glm::scale(glm::vec3(scale, scale, scale));

//glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
glm::mat4 modelMatrix = glm::mat4(1.0f);
//-------------//


//ViewMatrix//
glm::vec3 cameraPosition = glm::vec3(3, 3, 3); // Camera position in World Space
glm::vec3 lookAtPosition = glm::vec3(0, 0, 0); // Position to look at

glm::mat4 viewMatrix = glm::lookAt(cameraPosition, lookAtPosition, glm::vec3(0.0f, 1.0f, 0.0f));
//----------//


//ProjectionMatrix//
float fov = 45; //field of view
float aspectRatio = (float)WIDTH / (float)HEIGHT; //aspectRatio
float near = 0.1f; //near
float far = 100; //far

glm::mat4 projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, near, far);
//----------------//


//MVP//
glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
//---//

#pragma endregion


int main(void) 
{
	glewExperimental = true;

	if (!glfwInit())
	{
		return -1;
	}

    cout << lightPosLoc;

	// 4x antialiasing
	glfwWindowHint(GLFW_SAMPLES, 4);

	monitor = glfwGetPrimaryMonitor();

	window = glfwCreateWindow(WIDTH, HEIGHT, "ProjetoP3D", NULL, NULL);
	
	if (window == NULL) 
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glewExperimental = true;

	if (glewInit() != GLEW_OK) 
	{
		return -1;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	init();

	do 
	{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);
	
		// Draw
		display();

		//LightControl();

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} 
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &uvBuffer);
	glDeleteBuffers(1, &normalBuffer);
	glDeleteProgram(programID);
	
	//LIGHTNING
	glDeleteProgram(programa);

	glDeleteTextures(1, &textureID);
	glDeleteVertexArrays(1, &vertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

void init(void) 
{
	// Background
	glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	glGenVertexArrays(1, &vertexArrayID);
	glBindVertexArray(vertexArrayID);

	// Read our .obj file
	bool res = loadOBJ("Model/Iron_Man.obj", vertices, uvs, normals);

	//Load obj textures
	loadTexture("Model/Iron_Man_D.tga");

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("shaders/triangles.vert", "shaders/triangles.frag");
	
	//LIGHTNING
	programa = LoadShaders("shaders/Light.vert", "shaders/Light.frag");

	// Get a handle for our "MVP" uniform
	matrixID = glGetUniformLocation(programID, "MVP");
	viewMatrixID = glGetUniformLocation(programID, "V");
	modelMatrixID = glGetUniformLocation(programID, "M");

	// Get a handle for our "LightPosition" uniform
	lightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	// Get a handle for our "myTexture" uniform
	textureID = glGetUniformLocation(programID, "myTexture");

	//SPOTLIGHT
	lightPosLoc = glGetUniformLocation(programID, "light.position");
	lightSpotDirLoc = glGetUniformLocation(programID, "light.direction");
	lightSpotCutOffLoc = glGetUniformLocation(programID, "light.cutOff");
	viewPosLoc = glGetUniformLocation(programID, "light.viewPos");
	lightSpotOuterCutOffLoc = glGetUniformLocation(programID, "light.outerCutOff");


	//Load into a VBO
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	// Vertex Buffer
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	// UV Buffer
	glGenBuffers(1, &uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	// NormalBuffer
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// IndexBuffer
	glGenBuffers(1, &elementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);


	glUseProgram(programID);
	
	//LIGHTNING
	glUseProgram(programa);
}

void display(void) 
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);

	//LIGHTNING
	glUseProgram(programa);

	//viewMatrix based on inputs
	computeMatrixFromInputs(window);
	viewMatrix = getViewMatrix();
	mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;

	glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvpMatrix[0][0]);
	glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
	glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &viewMatrix[0][0]);

	//LIGHTNING
	//ERA SUPOSTO P�R ISTO NUMA FUN��O MAS ESTA MERDA N�O D� PARA CHAMAR FUN��ES NA MAIN
	//--//
	glm::vec3 lightPos;

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		std::cout << "Pressed 1" << std::endl;
		lightPos = glm::vec3(7, 4, 4);
		glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);
	}

	else 
	{
		lightPos = glm::vec3(1, 2, 3);
		glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);
	}
	//--//

	//glm::vec3 lightPos = glm::vec3(7, 4, 4);
	//glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);

	//SPOTLIGHT
	/*glUniform3d(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
	glUniform3d(lightSpotDirLoc, lookAtPosition.z, lookAtPosition.y, lookAtPosition.z);
	glUniform3d(viewPosLoc, lookAtPosition.z, lookAtPosition.y, lookAtPosition.z);
	glUniform1f(lightSpotCutOffLoc, glm::cos(glm::radians(12.5f)));
	glUniform1f(lightSpotOuterCutOffLoc, glm::cos(glm::radians(17.5f)));*/

	// Attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Attribute buffer : uvs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0); //attribute, size, normalized?, stride, array buffer offset

	// Attribute buffer : normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

	// Draw the triangles
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT,(void*)0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);


	//LIGHTNING
	// Fonte de luz ambiente global
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "ambientLight.ambient"), 1, glm::value_ptr(glm::vec3(0.1, 0.1, 0.1)));

	// Fonte de luz direcional
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "directionalLight.direction"), 1, glm::value_ptr(glm::vec3(1.0, 0.0, 0.0)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "directionalLight.ambient"), 1, glm::value_ptr(glm::vec3(0.2, 0.2, 0.2)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "directionalLight.diffuse"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "directionalLight.specular"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));

	// Fonte de luz pontual #1
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[0].position"), 1, glm::value_ptr(glm::vec3(0.0, 0.0, 5.0)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[0].ambient"), 1, glm::value_ptr(glm::vec3(0.1, 0.1, 0.1)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[0].diffuse"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[0].specular"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
	glProgramUniform1f(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[0].constant"), 1.0f);
	glProgramUniform1f(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[0].linear"), 0.06f);
	glProgramUniform1f(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[0].quadratic"), 0.02f);

	// Fonte de luz pontual #2
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[1].position"), 1, glm::value_ptr(glm::vec3(-2.0, 2.0, 5.0)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[1].ambient"), 1, glm::value_ptr(glm::vec3(0.1, 0.1, 0.1)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[1].diffuse"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[1].specular"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
	glProgramUniform1f(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[1].constant"), 1.0f);
	glProgramUniform1f(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[1].linear"), 0.06f);
	glProgramUniform1f(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "pointLight[1].quadratic"), 0.02f);

	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "material.emissive"), 1, glm::value_ptr(glm::vec3(0.0, 0.0, 0.0)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "material.ambient"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "material.diffuse"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
	glProgramUniform3fv(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "material.specular"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
	glProgramUniform1f(programa, glGetProgramResourceLocation(programa, GL_UNIFORM, "material.shininess"), 12.0f);
	
	//glClear(GL_COLOR_BUFFER_BIT);
}


/*void print_gl_info() 
{
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	cout << "\nOpenGL version " << major << '.' << minor << endl;

	const GLubyte *glversion = glGetString(GL_VERSION);
	const GLubyte *glvendor = glGetString(GL_VENDOR);
	const GLubyte *glrenderer = glGetString(GL_RENDERER);
	cout << "\nVersion:  " << glversion << endl <<
		"Vendor:   " << glvendor << endl <<
		"Renderer: " << glrenderer << endl;

	cout << "\nSupported GLSL versions:\n";
	const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	cout << "Higher supported version:\n" << glslversion << endl;
	
	GLint numglslversions;
	cout << "Other supported versions:\n";
	glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &numglslversions);

	for (int n = 0; n < numglslversions; n++) {
		cout << glGetStringi(GL_SHADING_LANGUAGE_VERSION, n) << endl;
	}
}*/

void print_error(int error, const char *description)
{
	cout << description << endl;
}

void loadTexture(string textureFile) 
{
	glGenTextures(1, &textureID);

	// Bind the newly created texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	glUniform1i(textureID, 0);

	// Nice trilinear filtering.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Activates image inversion
	stbi_set_flip_vertically_on_load(true);

	int width, height, nChannels;
	unsigned char *imageData = stbi_load(textureFile.c_str(), &width, &height, &nChannels, 0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);

	// Frees the image from memory
	stbi_image_free(imageData);
}

void LightControl()
{
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		std::cout << "Pressed 1" << std::endl;
		glm::vec3 lightPos = glm::vec3(7, 4, 4);
	}

	else
	{
		glm::vec3 lightPos = glm::vec3(0, 0, 0);
	}

	glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);
}