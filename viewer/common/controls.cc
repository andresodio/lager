#include <stdio.h>

// Include GLFW
#include <glfw3.h>
extern GLFWwindow* g_window;  // The "extern" keyword here is to access the variable "window" declared in tutorialXXX.cpp. This is a hack to keep the tutorials simple. Please avoid this.

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "controls.h"

glm::mat4 g_view_matrix;
glm::mat4 g_projection_matrix;

glm::mat4 GetViewMatrix() {
  return g_view_matrix;
}

glm::mat4 GetProjectionMatrix() {
  return g_projection_matrix;
}

// Initial position : on +Y and +Z
glm::vec3 position = glm::vec3(0, 0, 10);

// Initial horizontal angle : toward -Z
float g_horizontal_angle = 3.14f * 2.0f;

// Initial vertical angle : none
float g_vertical_angle = 3.14f / 1.0f;

// Initial Field of View
float g_initial_fov = 45.0f;

float g_speed = 1.25f;  // 1.25 units / second
float g_mouse_speed = 0.002f;

void ComputeMatricesFromInputs() {
  // glfwGetTime is called only once, the first time this function is called
  static double last_time = glfwGetTime();

  // Compute time difference between current and last frame
  double current_time = glfwGetTime();
  float delta_time = float(current_time - last_time);

  // Get mouse position
  double x_pos, y_pos;
  glfwGetCursorPos(g_window, &x_pos, &y_pos);

  // Reset mouse position for next frame
  glfwSetCursorPos(g_window, 0, 0);

  // Compute new orientation
  g_horizontal_angle += g_mouse_speed * float(-x_pos);
  g_vertical_angle += g_mouse_speed * float(-y_pos);

  // Direction : Spherical coordinates to Cartesian coordinates conversion
  glm::vec3 direction(cos(-g_vertical_angle) * sin(g_horizontal_angle),
                      sin(-g_vertical_angle),
                      cos(-g_vertical_angle) * cos(g_horizontal_angle));

  // Right vector
  glm::vec3 right = glm::vec3(sin(g_horizontal_angle + 3.14f / 2.0f), 0,
                              cos(g_horizontal_angle + 3.14f / 2.0f));

  // Up vector
  glm::vec3 up = glm::cross(right, direction);

  // Move forward
  if (glfwGetKey(g_window, GLFW_KEY_UP) == GLFW_PRESS
      || glfwGetKey(g_window, 'W') == GLFW_PRESS) {
    position += direction * delta_time * g_speed;
  }
  // Move backward
  if (glfwGetKey(g_window, GLFW_KEY_DOWN) == GLFW_PRESS
      || glfwGetKey(g_window, 'S') == GLFW_PRESS) {
    position -= direction * delta_time * g_speed;
  }
  // Strafe left
  if (glfwGetKey(g_window, GLFW_KEY_LEFT) == GLFW_PRESS
      || glfwGetKey(g_window, 'A') == GLFW_PRESS) {
    position -= right * delta_time * g_speed;
  }
  // Strafe right
  if (glfwGetKey(g_window, GLFW_KEY_RIGHT) == GLFW_PRESS
      || glfwGetKey(g_window, 'D') == GLFW_PRESS) {
    position += right * delta_time * g_speed;
  }

  float FoV = g_initial_fov;  // - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.

  // Projection matrix : 45° Field of View, 5:4 ratio, display range : 0.1 unit <-> 100 units
  g_projection_matrix = glm::perspective(FoV, 5.0f / 4.0f, 0.1f, 100.0f);
  // Camera matrix
  g_view_matrix = glm::lookAt(position,           // Camera is here
      position + direction,  // and looks here : at the same position, plus "direction"
      up                  // Head is up (set to 0,1,0)
      );

  // For the next frame, the "last time" will be "now"
  last_time = current_time;
}
