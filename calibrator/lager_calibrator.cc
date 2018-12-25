#include <iostream>
#include <string>
#include <fstream>
#include <time.h> // for nanosleep

#include <osvr/ClientKit/Context.h>
#include <osvr/ClientKit/Interface.h>

using std::cin;
using std::cout;
using std::endl;
using std::ofstream;
using std::string;

#define MIN_GRAB_VALUE 0.82

/* Globals */

double g_leftmost_position = 0;
double g_rightmost_position = 0;

/// OSVR context
osvr::clientkit::ClientContext g_context("LagerCalibrator");

/// Pointer to the current tracker structures
osvr::clientkit::Interface g_left_tracker;
osvr::clientkit::Interface g_right_tracker;

/// Pointer to the current button structures
osvr::clientkit::Interface g_left_button;
osvr::clientkit::Interface g_right_button;

/// Pointer to the current grab structures
osvr::clientkit::Interface g_left_grab;
osvr::clientkit::Interface g_right_grab;

/// Boolean to determine whether we should save movement positions
bool save_positions = false;

/// Boolean to keep track of the button (or grab) release
bool button_released = false;

/**
 * Prints a welcome banner for the LaGeR Calibrator.
 */
void PrintWelcomeBanner() {
  cout << " ________________________________ " << endl;
  cout << "|                                |" << endl;
  cout << "|        LAGER CALIBRATOR        |" << endl;
  cout << "|________________________________|" << endl;
  cout << "                                  " << endl;
}

/**
 * Prints the options menu for the LaGeR Calibrator.
 */
void PrintOptionsMenu() {
  cout << "Please choose an option:" << endl;
  cout << endl;
  cout << "1. Calibrate sensor" << endl;
  cout << endl;
  cout << "2. Quit" << endl;
  cout << endl;
  cout << "Choice: ";
}

/**
 * Callback that handles changes to the sensor positions.
 */
void HandleTrackerChange(void *user_data,
                        const OSVR_TimeValue *time_value,
                        const OSVR_PositionReport *cur_report) {
  if (save_positions) {
    double current_position = cur_report->xyz.data[0];
    if (current_position < g_leftmost_position) {
      g_leftmost_position = current_position;
    } else if (current_position > g_rightmost_position) {
      g_rightmost_position = current_position;
    }
  }
}

/**
 * Callback that handles changes to a sensor button states.
 */
void HandleButtonChange(void *user_data, const OSVR_TimeValue *time_value,
                      const OSVR_ButtonReport *cur_report) {
  bool previously_saved_positions = save_positions;
  save_positions = cur_report->state ? true : false;
  if (previously_saved_positions && !save_positions) {
    button_released = true;
  }
}

/**
 * Callback that handles changes to a sensor grab state.
 */
void HandleGrabChange(void *user_data, const OSVR_TimeValue *time_value,
                      const OSVR_AnalogReport *cur_report) {
  bool previously_saved_positions = save_positions;
  save_positions = (cur_report->state > MIN_GRAB_VALUE) ? true : false;
  if (previously_saved_positions && !save_positions) {
    button_released = true;
  }
}

void InitializeTrackers() {
  // Initialize the tracker handlers
  g_left_tracker = g_context.getInterface("/me/hands/left");
  g_right_tracker = g_context.getInterface("/me/hands/right");

  g_left_tracker.registerCallback(&HandleTrackerChange, NULL);
  g_right_tracker.registerCallback(&HandleTrackerChange, NULL);

  // Initialize the button and grab handlers
  g_left_button = g_context.getInterface("/controller/left/1");
  g_right_button = g_context.getInterface("/controller/right/1");

  g_left_button.registerCallback(&HandleButtonChange, NULL);
  g_right_button.registerCallback(&HandleButtonChange, NULL);

  //g_left_grab = g_context.getInterface("/controller/left/trigger");
  g_right_grab = g_context.getInterface("/controller/right/trigger");

  //g_left_grab.registerCallback(&HandleGrabChange, NULL);
  g_right_grab.registerCallback(&HandleGrabChange, NULL);
}

/**
 * Writes the scale factor to a file to be used by liblager_convert
 */
void WriteScaleToFile(double scale) {
  ofstream sensor_scale_file;

  sensor_scale_file.open("/tmp/sensor_scale.cfg", std::ofstream::out | std::ofstream::trunc);
  if (!sensor_scale_file.is_open()) {
    cout << "Error writing to sensor scale file!" << endl;
    return;
  }

  sensor_scale_file << scale << endl;
}

/**
 * Calibrates a sensor by recording the leftmost and rightmost position values
 * and saving a scaling factor accordingly.
 */
void CalibrateSensor() {
  cout << "Move your sensor as far left as you will comfortably use it, then"
      " hold its button (or grab it) while you move it as far right as you will"
      " comfortably use it." << endl << std::flush;

  InitializeTrackers();

  struct timespec sleep_interval = { 0, 10000000 }; // 10 ms

  while (true) {
    // Request an update from the sensor context
    g_context.update();

    if (button_released) {
      break;
    }

    // Sleep so we don't take up 100% of CPU
    nanosleep(&sleep_interval, NULL);
  }

  cout << "Range: " << g_leftmost_position << " - " << g_rightmost_position << endl;
  double scale = 0.001 * (g_rightmost_position - g_leftmost_position);
  cout << "Scale: " << scale << endl;

  WriteScaleToFile(scale);
}

/**
 * The main loop of the LaGeR Calibrator.
 */
int main(int argc, const char *argv[]) {
  string input;
  std::string::size_type sz;   // alias of size_t
  bool quit = false;

  PrintWelcomeBanner();

  do{
      PrintOptionsMenu();
      getline(cin, input);
      switch (atoi(input.c_str())) {
        case 1:
          cout << endl;
          CalibrateSensor();
          cout << endl;
          break;
        case 2:
          quit = true;
          break;
      }
    } while(!quit);

  exit(0);
}
