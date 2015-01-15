// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <cstring>

#include "letter_coordinates.h"
#include "string_tokenizer.h"

using namespace std;

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.h>
//#include <common/texture.h>
#include <common/controls.h>

/* Constants */
#define STD_RADIUS 0.1
#define NO_ERROR 0

/* Forward declarations */
GLfloat getXVector (const double aPhi, const double aTheta);
GLfloat getYVector (const double aPhi, const double aTheta);
GLfloat getZVector (const double aTheta);

/*
 * Standard convention is:
 * x = r * sin THETA * cos PHI
 * y = r * sin THETA * sin PHI
 * z = r * cos THETA
 *
 * Here we set r = 0.1 and swap axes for OpenGL such that:
 * Std ---> OpenGL
 *  x  --->   z
 *  y  --->   x
 *  z  --->   y
 */

void getVectors(GLfloat &aDeltaX, GLfloat &aDeltaY, GLfloat &aDeltaZ, const double aPhi, const double aTheta)
{
	aDeltaX = getYVector(aPhi, aTheta);
	aDeltaY = getZVector(aTheta);
	aDeltaZ = getXVector(aPhi, aTheta);
}

GLfloat getXVector (const double aPhi, const double aTheta)
{
	return STD_RADIUS * sin(aTheta) * cos(aPhi);
}

GLfloat getYVector (const double aPhi, const double aTheta)
{
	return STD_RADIUS * sin(aTheta) * sin(aPhi);
}

GLfloat getZVector (const double aTheta)
{
	return STD_RADIUS * cos(aTheta);
}

void populateSensorVertexBuffers(int aSensorIndex, const vector<string>& aMovementsPairs, GLfloat* aSensorVertexBufferData) {
	//printf("sizeof(aSensorVertexBufferData): %li\n", sizeof(aSensorVertexBufferData));
	//printf("numVertexBufferElems: %li\n", numVertexBufferElems);
	GLfloat tempCoordinates[3] = { 0.0f, 0.0f, 0.0f };
	aSensorVertexBufferData[0] = 0.0f;
	aSensorVertexBufferData[1] = 0.0f;
	aSensorVertexBufferData[2] = 0.0f;
	for (int i = 0; i < aMovementsPairs.size(); i++) {
		int j = i * 3;
		string currentMovementPair = aMovementsPairs[i];
		//cout << currentMovementPair << endl;
		char currentMovement = currentMovementPair.c_str()[aSensorIndex];
		//cout << currentMovement << endl;
		/*
		 * Spherical gesture coordinates in PHI, THETA format
		 */
		GLfloat deltaX, deltaY, deltaZ;
		if (currentMovement == '_') {
			deltaX = 0.0f;
			deltaY = 0.0f;
			deltaZ = 0.0f;
		} else {
			struct sphericalCoordinates currentCoordinates =
					letterCoordinates[currentMovement];
			getVectors(deltaX, deltaY, deltaZ,
					degreesToRadians(currentCoordinates.mPhi),
					degreesToRadians(currentCoordinates.mTheta));
		}
		//cout << "movement " << currentS1Movement << " dx: " << deltaX << " dy: " << deltaY << " dz: " << deltaZ << endl;
		tempCoordinates[0] += deltaX;
		tempCoordinates[1] += deltaY;
		tempCoordinates[2] += deltaZ;
		aSensorVertexBufferData[j + 3] = tempCoordinates[0];
		aSensorVertexBufferData[j + 4] = tempCoordinates[1];
		aSensorVertexBufferData[j + 5] = tempCoordinates[2];
		//printf("vertex[%i, %i, %i] = {%f, %f, %f}\n", j+3, j+4, j+5, tempCoordinates[0], tempCoordinates[1], tempCoordinates[2]);
	}
}

void populateSensorColorBuffers(GLfloat* aSensor0ColorBufferData, GLfloat* aSensor1ColorBufferData, int aNumVertexBufferElems)
{
	int numMovements = aNumVertexBufferElems / 3;
	int numColorIntensityJumps = numMovements - 1;
	float colorIntensityInterval = 1.0f / numColorIntensityJumps;

	GLfloat sensor0TempIntensities[3] = { 0.0f, 0.0f, 0.0f }; // R,G,B
	GLfloat sensor1TempIntensities[3] = { 0.0f, 0.0f, 0.0f }; // R,G,B

	/* Origin is colored with intensity 0.0f */
	for (int i = 0; i < 3; i++) {
		aSensor0ColorBufferData[i] = 0.0f;
		aSensor1ColorBufferData[i] = 0.0f;
	}

	for (int i = 3; i < (aNumVertexBufferElems-2); i+=3) {
		sensor0TempIntensities[0] += colorIntensityInterval; // Intensify red for sensor 0
		sensor1TempIntensities[2] += colorIntensityInterval; // Intensify blue for sensor 1

		aSensor0ColorBufferData[i] = sensor0TempIntensities[0];
		aSensor0ColorBufferData[i+1] = sensor0TempIntensities[1];
		aSensor0ColorBufferData[i+2] = sensor0TempIntensities[2];

		aSensor1ColorBufferData[i] = sensor1TempIntensities[0];
		aSensor1ColorBufferData[i+1] = sensor1TempIntensities[1];
		aSensor1ColorBufferData[i+2] = sensor1TempIntensities[2];
	}
}

void readGestureFromArguments(char* argv[], vector<string>& movementPairs) {
	string gesture(argv[2]);

	tokenizeString(gesture, movementPairs, ".");
	cout << "Gesture: " << gesture << endl;
}

int readGestureFromFile(vector<string>& movementPairs) {
	ifstream gestureFile;
	gestureFile.open("gesture.dat");
	if (!gestureFile.is_open()) {
		return EXIT_FAILURE;
	}
	string line;
	if (!getline(gestureFile, line)) {
		return EXIT_FAILURE;
	}

	tokenizeString(line, movementPairs, ".");
	cout << "Gesture: " << line << endl;

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);


	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1280, 1024, "Gesture visualizer", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetCursorPos(window, 1280/2, 1024/2);

	// White background
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	// Thick lines
	glLineWidth(3.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);

	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// Get a handle for our buffers
	GLuint vertexPosition_modelspaceID = glGetAttribLocation(programID, "vertexPosition_modelspace");
	GLuint colorIntensityID = glGetAttribLocation(programID, "colorIntensity");
	//GLuint vertexUVID = glGetAttribLocation(programID, "vertexUV");
	
	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	vector<string> movementPairs;

	if ((argc > 2) && (strncmp(argv[1], "--gesture", strlen("--gesture")) == 0)) {
		readGestureFromArguments(argv, movementPairs);
	} else {
		if (readGestureFromFile(movementPairs) != NO_ERROR) {
			cout << "Error reading gesture from file, exiting." << endl;
			return -1;
		}
	}

	long int numVertexBufferElems = (movementPairs.size()+1) * 3; // One extra entry at the beginning for coordinate {0.0f, 0.0f, 0.0f}

	GLfloat* sensor0VertexBufferData = new GLfloat[numVertexBufferElems];
	GLfloat* sensor0ColorBufferData = new GLfloat[numVertexBufferElems];
	GLfloat* sensor1VertexBufferData = new GLfloat[numVertexBufferElems];
	GLfloat* sensor1ColorBufferData = new GLfloat[numVertexBufferElems];

	populateSensorVertexBuffers(0, movementPairs, sensor0VertexBufferData);
	populateSensorVertexBuffers(1, movementPairs, sensor1VertexBufferData);
	populateSensorColorBuffers(sensor0ColorBufferData, sensor1ColorBufferData, numVertexBufferElems);

	/* Vertex buffers */

	GLuint sensor0VertexBuffer;
	glGenBuffers(1, &sensor0VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sensor0VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertexBufferElems * sizeof(GLfloat), sensor0VertexBufferData, GL_STATIC_DRAW);

	GLuint sensor1VertexBuffer;
	glGenBuffers(1, &sensor1VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sensor1VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertexBufferElems * sizeof(GLfloat), sensor1VertexBufferData, GL_STATIC_DRAW);

	/* Color buffers */

	GLuint sensor0ColorBuffer;
	glGenBuffers(1, &sensor0ColorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sensor0ColorBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertexBufferElems * sizeof(GLfloat), sensor0ColorBufferData, GL_STATIC_DRAW);

	GLuint sensor1ColorBuffer;
	glGenBuffers(1, &sensor1ColorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sensor1ColorBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertexBufferElems * sizeof(GLfloat), sensor1ColorBufferData, GL_STATIC_DRAW);

	do{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// Enabled vertex attributes
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glEnableVertexAttribArray(colorIntensityID);

		glBindBuffer(GL_ARRAY_BUFFER, sensor0VertexBuffer);
		glVertexAttribPointer(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		glBindBuffer(GL_ARRAY_BUFFER, sensor0ColorBuffer);
		glVertexAttribPointer(
			colorIntensityID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Draw the gesture for sensor 0
		glDrawArrays(GL_LINE_STRIP, 0, movementPairs.size()+1); // Initial position + movement pairs read from the file

		glBindBuffer(GL_ARRAY_BUFFER, sensor1VertexBuffer);
		glVertexAttribPointer(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		glBindBuffer(GL_ARRAY_BUFFER, sensor1ColorBuffer);
		glVertexAttribPointer(
			colorIntensityID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Draw the gesture for sensor 1
		glDrawArrays(GL_LINE_STRIP, 0, movementPairs.size()+1); // Initial position + movement pairs read from the file

		glDisableVertexAttribArray(vertexPosition_modelspaceID);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Clean up buffers and shader
	glDeleteBuffers(1, &sensor0VertexBuffer);
	glDeleteBuffers(1, &sensor1VertexBuffer);

	glDeleteBuffers(1, &sensor0ColorBuffer);
	glDeleteBuffers(1, &sensor1ColorBuffer);

	glDeleteProgram(programID);
	glDeleteTextures(1, &TextureID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

