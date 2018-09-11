// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <time.h> // for nanosleep
#include <cstring>

#include "letter_coordinates.h"
#include "string_tokenizer.h"

using namespace std;

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* g_window;

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
GLfloat GetXVector(const double theta, const double phi);
GLfloat GetYVector(const double theta, const double phi);
GLfloat GetZVector(const double theta);

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

/**
 * Converts theta (polar) and phi (azimuthal) angles into normalized X, Y, and
 * Z vector components.
 */
void GetVectors(GLfloat &delta_x, GLfloat &delta_y, GLfloat &delta_z,
                const double theta, const double phi) {
  delta_x = GetYVector(theta, phi);
  delta_y = GetZVector(theta);
  delta_z = GetXVector(theta, phi);
}

/**
 * Converts theta (polar) and phi (azimuthal) angles into a normalized X vector
 * component.
 */
GLfloat GetXVector(const double theta, const double phi) {
  return STD_RADIUS * sin(theta) * cos(phi);
}

/**
 * Converts theta (polar) and phi (azimuthal) angles into a normalized Y vector
 * component.
 */
GLfloat GetYVector(const double theta, const double phi) {
  return STD_RADIUS * sin(theta) * sin(phi);
}

/**
 * Converts theta (polar) and phi (azimuthal) angles into a normalized Z vector
 * component.
 */
GLfloat GetZVector(const double theta) {
  return STD_RADIUS * cos(theta);
}

/**
 * Takes a sensor index and a reference to a vector of movement pair strings,
 * and uses them to populate an array of sensor vertex buffer data.
 */
void PopulateSensorVertexBuffers(int sensor_index,
                                 const vector<string>& movement_pairs,
                                 GLfloat* sensor_vertex_buffer_data) {
  //printf("sizeof(sensor_vertex_buffer_data): %li\n", sizeof(sensor_vertex_buffer_data));
  //printf("num_vertex_buffer_elems: %li\n", num_vertex_buffer_elems);
  GLfloat temp_coordinates[3] = { 0.0f, 0.0f, 0.0f };
  sensor_vertex_buffer_data[0] = 0.0f;
  sensor_vertex_buffer_data[1] = 0.0f;
  sensor_vertex_buffer_data[2] = 0.0f;
  for (int i = 0; i < movement_pairs.size(); i++) {
    int j = i * 3;
    string current_movement_pair = movement_pairs[i];
    //cout << current_movement_pair << endl;
    char current_movement = current_movement_pair.c_str()[sensor_index];
    //cout << current_movement << endl;
    /*
     * Spherical gesture coordinates in PHI, THETA format
     */
    GLfloat delta_x, delta_y, delta_z;
    if (current_movement == '_') {
      delta_x = 0.0f;
      delta_y = 0.0f;
      delta_z = 0.0f;
    } else {
      struct SphericalCoordinates current_coordinates =
          letter_coordinates[current_movement];
      GetVectors(delta_x, delta_y, delta_z,
                 DegreesToRadians(current_coordinates.theta),
                 DegreesToRadians(current_coordinates.phi));
    }
    //cout << "movement " << currentS1Movement << " dx: " << deltaX << " dy: " << deltaY << " dz: " << deltaZ << endl;
    temp_coordinates[0] += delta_x;
    temp_coordinates[1] += delta_y;
    temp_coordinates[2] += delta_z;
    sensor_vertex_buffer_data[j + 3] = temp_coordinates[0];
    sensor_vertex_buffer_data[j + 4] = temp_coordinates[1];
    sensor_vertex_buffer_data[j + 5] = temp_coordinates[2];
    //printf("vertex[%i, %i, %i] = {%f, %f, %f}\n", j+3, j+4, j+5, temp_coordinates[0], temp_coordinates[1], temp_coordinates[2]);
  }
}

/**
 * Populates a pair of sensor color buffer data arrays such that sensor 0
 * becomes progressively redder and sensor 1 progressively bluer.
 */
void PopulateSensorColorBuffers(GLfloat* sensor_0_color_buffer_data,
                                GLfloat* sensor_1_color_buffer_data,
                                int anum_vertex_buffer_elems) {
  int num_movements = anum_vertex_buffer_elems / 3;
  int num_color_intensity_jumps = num_movements - 1;
  float color_intensity_interval = 1.0f / num_color_intensity_jumps;

  GLfloat sensor_0_temp_intensities[3] = { 0.0f, 0.0f, 0.0f };  // R,G,B
  GLfloat sensor_1_temp_intensities[3] = { 0.0f, 0.0f, 0.0f };  // R,G,B

  /* Origin is colored with intensity 0.0f */
  for (int i = 0; i < 3; i++) {
    sensor_0_color_buffer_data[i] = 0.0f;
    sensor_1_color_buffer_data[i] = 0.0f;
  }

  for (int i = 3; i < (anum_vertex_buffer_elems - 2); i += 3) {
    sensor_0_temp_intensities[0] += color_intensity_interval;  // Intensify red for sensor 0
    sensor_1_temp_intensities[2] += color_intensity_interval;  // Intensify blue for sensor 1

    sensor_0_color_buffer_data[i] = sensor_0_temp_intensities[0];
    sensor_0_color_buffer_data[i + 1] = sensor_0_temp_intensities[1];
    sensor_0_color_buffer_data[i + 2] = sensor_0_temp_intensities[2];

    sensor_1_color_buffer_data[i] = sensor_1_temp_intensities[0];
    sensor_1_color_buffer_data[i + 1] = sensor_1_temp_intensities[1];
    sensor_1_color_buffer_data[i + 2] = sensor_1_temp_intensities[2];
  }
}

/**
 * Takes a gesture from the program arguments, tokenizes it, and writes the
 * tokens to a referenced vector of movement pair strings.
 */
void ReadGestureFromArguments(char* argv[], vector<string>& movement_pairs) {
  string gesture(argv[2]);

  TokenizeString(gesture, movement_pairs, ".");
  cout << "Gesture: " << gesture << endl;
}

/**
 * Parses the gestures.dat file and writes its first gesture into a referenced
 * vector of movement pair strings.
 */
int ReadGestureFromFile(vector<string>& movement_pairs) {
  ifstream gesture_file;
  gesture_file.open("gesture.dat");
  if (!gesture_file.is_open()) {
    return EXIT_FAILURE;
  }
  string line;
  if (!getline(gesture_file, line)) {
    return EXIT_FAILURE;
  }

  TokenizeString(line, movement_pairs, ".");
  cout << "Gesture: " << line << endl;

  return EXIT_SUCCESS;
}

/**
 * The main loop of the LaGeR Viewer.
 */
int main(int argc, char *argv[]) {
  // Initialise GLFW
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  struct timespec sleep_interval = { 0, 10000000 }; // 10 ms

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  // Open a window and create its OpenGL context
  g_window = glfwCreateWindow(1280, 1024, "Gesture visualizer", NULL, NULL);
  if (g_window == NULL) {
    fprintf(stderr, "Failed to open GLFW window.\n");
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(g_window);

  // Initialize GLEW
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(g_window, GLFW_STICKY_KEYS, GL_TRUE);
  glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // Consume spurious initial mouse events and center the mouse in the viewer window
  for (int i = 0; i < 3; i++) {
    glfwPollEvents();
  }
  glfwSetCursorPos(g_window, 0, 0);

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
  GLuint program_id = LoadShaders(
      "/usr/local/include/TransformVertexShader.vertexshader",
      "/usr/local/include/TextureFragmentShader.fragmentshader");

  // Get a handle for our "MVP" uniform
  GLuint matrix_id = glGetUniformLocation(program_id, "MVP");

  // Get a handle for our buffers
  GLuint vertex_position_model_space_id = glGetAttribLocation(
      program_id, "vertexPosition_modelspace");
  GLuint color_intensity_id = glGetAttribLocation(program_id, "colorIntensity");
  //GLuint vertex_uv_id = glGetAttribLocation(programID, "vertexUV");

  // Get a handle for our "myTextureSampler" uniform
  GLuint texture_id = glGetUniformLocation(program_id, "myTextureSampler");

  vector < string > movement_pairs;

  if ((argc > 2) && (strncmp(argv[1], "--gesture", strlen("--gesture")) == 0)) {
    ReadGestureFromArguments(argv, movement_pairs);
  } else {
    if (ReadGestureFromFile(movement_pairs) != NO_ERROR) {
      cout << "Error reading gesture from file, exiting." << endl;
      return -1;
    }
  }

  long int num_vertex_buffer_elems = (movement_pairs.size() + 1) * 3;  // One extra entry at the beginning for coordinate {0.0f, 0.0f, 0.0f}

  GLfloat* sensor_0_vertex_buffer_data = new GLfloat[num_vertex_buffer_elems];
  GLfloat* sensor_0_color_buffer_data = new GLfloat[num_vertex_buffer_elems];
  GLfloat* sensor_1_vertex_buffer_data = new GLfloat[num_vertex_buffer_elems];
  GLfloat* sensor_1_color_buffer_data = new GLfloat[num_vertex_buffer_elems];

  PopulateSensorVertexBuffers(0, movement_pairs, sensor_0_vertex_buffer_data);
  PopulateSensorVertexBuffers(1, movement_pairs, sensor_1_vertex_buffer_data);
  PopulateSensorColorBuffers(sensor_0_color_buffer_data,
                             sensor_1_color_buffer_data,
                             num_vertex_buffer_elems);

  /* Vertex buffers */

  GLuint sensor_0_vertex_buffer;
  glGenBuffers(1, &sensor_0_vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, sensor_0_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, num_vertex_buffer_elems * sizeof(GLfloat),
               sensor_0_vertex_buffer_data, GL_STATIC_DRAW);

  GLuint sensor_1_vertex_buffer;
  glGenBuffers(1, &sensor_1_vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, sensor_1_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, num_vertex_buffer_elems * sizeof(GLfloat),
               sensor_1_vertex_buffer_data, GL_STATIC_DRAW);

  /* Color buffers */

  GLuint sensor_0_color_buffer;
  glGenBuffers(1, &sensor_0_color_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, sensor_0_color_buffer);
  glBufferData(GL_ARRAY_BUFFER, num_vertex_buffer_elems * sizeof(GLfloat),
               sensor_0_color_buffer_data, GL_STATIC_DRAW);

  GLuint sensor_1_color_buffer;
  glGenBuffers(1, &sensor_1_color_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, sensor_1_color_buffer);
  glBufferData(GL_ARRAY_BUFFER, num_vertex_buffer_elems * sizeof(GLfloat),
               sensor_1_color_buffer_data, GL_STATIC_DRAW);

  do {
    // Don't hog the CPU
    nanosleep(&sleep_interval, NULL);

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use our shader
    glUseProgram(program_id);

    // Compute the MVP matrix from keyboard and mouse input
    ComputeMatricesFromInputs();
    glm::mat4 projection_matrix = GetProjectionMatrix();
    glm::mat4 view_matrix = GetViewMatrix();
    glm::mat4 model_matrix = glm::mat4(1.0);
    glm::mat4 mvp = projection_matrix * view_matrix * model_matrix;

    // Send our transformation to the currently bound shader,
    // in the "MVP" uniform
    glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);

    // Enabled vertex attributes
    glEnableVertexAttribArray(vertex_position_model_space_id);
    glEnableVertexAttribArray(color_intensity_id);

    glBindBuffer(GL_ARRAY_BUFFER, sensor_0_vertex_buffer);
    glVertexAttribPointer(vertex_position_model_space_id,  // The attribute we want to configure
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*) 0            // array buffer offset
        );

    glBindBuffer(GL_ARRAY_BUFFER, sensor_0_color_buffer);
    glVertexAttribPointer(color_intensity_id,  // The attribute we want to configure
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*) 0            // array buffer offset
        );

    // Draw the gesture for sensor 0
    glDrawArrays(GL_LINE_STRIP, 0, movement_pairs.size() + 1);  // Initial position + movement pairs read from the file

    glBindBuffer(GL_ARRAY_BUFFER, sensor_1_vertex_buffer);
    glVertexAttribPointer(vertex_position_model_space_id,  // The attribute we want to configure
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*) 0            // array buffer offset
        );

    glBindBuffer(GL_ARRAY_BUFFER, sensor_1_color_buffer);
    glVertexAttribPointer(color_intensity_id,  // The attribute we want to configure
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*) 0            // array buffer offset
        );

    // Draw the gesture for sensor 1
    glDrawArrays(GL_LINE_STRIP, 0, movement_pairs.size() + 1);  // Initial position + movement pairs read from the file

    glDisableVertexAttribArray(vertex_position_model_space_id);

    // Swap buffers
    glfwSwapBuffers(g_window);
    glfwPollEvents();
  }  // Check if the ESC key was pressed or the window was closed
  while (glfwGetKey(g_window, GLFW_KEY_ESCAPE) != GLFW_PRESS
      && glfwWindowShouldClose(g_window) == 0);

  // Clean up buffers and shader
  glDeleteBuffers(1, &sensor_0_vertex_buffer);
  glDeleteBuffers(1, &sensor_1_vertex_buffer);

  glDeleteBuffers(1, &sensor_0_color_buffer);
  glDeleteBuffers(1, &sensor_1_color_buffer);

  glDeleteProgram(program_id);
  glDeleteTextures(1, &texture_id);

  // Close OpenGL window and terminate GLFW
  glfwTerminate();

  return 0;
}
