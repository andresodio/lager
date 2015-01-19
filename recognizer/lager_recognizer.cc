// phan_client.C - simplest example: generates a flat horizontal plane

#include <stdio.h>    // for printf, NULL
#include <math.h>     // for atan2
#include <time.h>     // for nanosleep
#include <algorithm>  // for std::min_element
#include <iostream>   // for std::cout
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>

#include <boost/math/common_factor.hpp>

#include <vrpn_Button.h>                // for vrpn_BUTTONCB, etc
#include <vrpn_Tracker.h>               // for vrpn_TRACKERCB, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_ForceDevice.h"           // for vrpn_ForceDevice_Remote, etc
#include "vrpn_Types.h"                 // for vrpn_float64

#include "spherical_coordinates.h"
#include "coordinates_letter.h"
#include "string_tokenizer.h"

#define RECOGNIZER_ERROR -1
#define RECOGNIZER_NO_ERROR 0

#define SENSOR_SERVER "Filter0@localhost"

#define DISTANCE_INTERVAL_SQUARED 0.0004f //0.4 * (1 cm)^2

#define MAIN_SLEEP_INTERVAL_MICROSECONDS 1000 // 1ms
#define MAIN_SLEEP_INTERVAL_MILLISECONDS MAIN_SLEEP_INTERVAL_MICROSECONDS/1000
#define GESTURE_PAUSE_TIME_MILLISECONDS 500
#define GESTURE_GROUPING_TIME_MILLISECONDS 200
#define SINGLE_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT 20
#define DUAL_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT 30

using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;

using std::chrono::duration;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::chrono::time_point;

/* Forward declarations */
int SnapAngle(double angle);

/* Globals */

static vrpn_TRACKERCB g_last_report_0;  // last report for sensor 0
static vrpn_TRACKERCB g_last_report_1;  // last report for sensor 1

stringstream g_gesture_string;

time_point<system_clock> g_global_last_movement_time = system_clock::now();

time_point<system_clock> g_sensor_0_last_movement_time = system_clock::now();
time_point<system_clock> g_sensor_0_last_grouped_movement_time =
    system_clock::now();

time_point<system_clock> g_sensor_1_last_movement_time = system_clock::now();
time_point<system_clock> g_sensor_1_last_grouped_movement_time =
    system_clock::now();

char g_last_sensor_0_letter = '_';
char g_last_sensor_1_letter = '_';

vrpn_float64 GetDeltaX(const vrpn_TRACKERCB& last_report,
                       const vrpn_TRACKERCB& tracker) {
  return (last_report.pos[1] - tracker.pos[1]);
}

vrpn_float64 GetDeltaY(const vrpn_TRACKERCB& last_report,
                       const vrpn_TRACKERCB& tracker) {
  return (tracker.pos[0] - last_report.pos[0]);
}

vrpn_float64 GetDeltaZ(const vrpn_TRACKERCB& last_report,
                       const vrpn_TRACKERCB& tracker) {
  return (tracker.pos[2] - last_report.pos[2]);
}

double GetMovementThetaInDegrees(vrpn_float64 delta_x, vrpn_float64 delta_y,
                                 vrpn_float64 delta_z) {
  double movementTheta = acos(
      delta_z
          / (sqrt(pow(delta_x, 2.0) + pow(delta_y, 2.0) + pow(delta_z, 2.0))))
      * (180.0 / M_PI);
  return fmod(movementTheta + 360.0, 360.0);  // Always returns a positive angle
}

double GetMovementPhiInDegrees(vrpn_float64 delta_x, vrpn_float64 delta_y,
                               double theta) {
  int snap_theta = SnapAngle(theta);
  if (snap_theta == 0 || snap_theta == 180) {
    return 0;  // Phi becomes meaningless at the north and south poles, so we default it to 0.
  }

  double movementPhi = atan2(delta_y, delta_x) * (180.0 / M_PI);
  return fmod(movementPhi + 360.0, 360.0);  // Always returns a positive angle
}

/* Snaps angle to 45 degree intervals */
int SnapAngle(double aAngle) {
  return 45 * (round(aAngle / 45.0));
}

vrpn_float64 GetDistanceSquared(const vrpn_TRACKERCB& last_report,
                                const vrpn_TRACKERCB& tracker) {
  return (last_report.pos[0] - tracker.pos[0])
      * (last_report.pos[0] - tracker.pos[0])
      + (last_report.pos[1] - tracker.pos[1])
          * (last_report.pos[1] - tracker.pos[1])
      + (last_report.pos[2] - tracker.pos[2])
          * (last_report.pos[2] - tracker.pos[2]);
}

#define abs(x) ((x)<0 ? -(x) : (x))

void PrintSensorCoordinates(const vrpn_TRACKERCB& last_report,
                            const vrpn_TRACKERCB& tracker) {
  printf("\nSensor %d is now at (%g,%g,%g)\n", tracker.sensor, tracker.pos[0],
         tracker.pos[1], tracker.pos[2]);
  printf("old/new X: %g, %g\n", last_report.pos[1], tracker.pos[1]);
  printf("old/new Y: %g, %g\n", last_report.pos[0], tracker.pos[0]);
  printf("old/new Z: %g, %g\n", last_report.pos[2], tracker.pos[2]);
}

char GetCurrentLetter(int snap_phi, int snap_theta) {
  struct SphericalCoordinates current_coordinates;
  current_coordinates.phi = snap_phi;
  current_coordinates.theta = snap_theta;
  return coordinates_letter[current_coordinates];
}

int GetMillisecondsUntilNow(const time_point<system_clock> &last_time) {
  time_point<system_clock> now = system_clock::now();
  return duration<double, std::milli>(now - last_time).count();
}

int GetMillisecondsSinceTrackerTime(const vrpn_TRACKERCB& tracker,
                                    const time_point<system_clock> &last_time) {
  time_point<system_clock> current_movement_time(
      seconds(tracker.msg_time.tv_sec)
          + microseconds(tracker.msg_time.tv_usec));
  return duration<double, std::milli>(current_movement_time - last_time).count();
}

void UpdateTimePoint(time_point<system_clock>& time_to_update,
                     time_point<system_clock> new_time) {
  time_to_update = new_time;
}

bool GesturePaused() {
  return (GetMillisecondsUntilNow(g_global_last_movement_time)
      > GESTURE_PAUSE_TIME_MILLISECONDS);
}

void ResetGestureString() {
  g_gesture_string.str("");
}

void CalculateMovementDeltas(const vrpn_TRACKERCB& tracker,
                             const vrpn_TRACKERCB* last_report,
                             vrpn_float64& delta_x, vrpn_float64& delta_y,
                             vrpn_float64& delta_z) {
  delta_x = GetDeltaX(*last_report, tracker);
  delta_y = GetDeltaY(*last_report, tracker);
  delta_z = GetDeltaZ(*last_report, tracker);
  //printf("X delta: %g, Y delta: %g, Z delta: %g\n", deltaX, deltaY, deltaZ);
}

void CalculateMovementAngles(double& theta, double& phi, int& snap_theta,
                             int& snap_phi, vrpn_float64 delta_x,
                             vrpn_float64 delta_y, vrpn_float64 delta_z) {
  theta = GetMovementThetaInDegrees(delta_x, delta_y, delta_z);
  snap_theta = SnapAngle(theta);
  phi = GetMovementPhiInDegrees(delta_x, delta_y, theta);
  snap_phi = SnapAngle(phi);
}

void MoveHeadToBeginningOfLetterPair() {
  if (g_gesture_string.str().size() >= 3) {
    g_gesture_string.seekp(-3, g_gesture_string.cur);
  }
}

time_point<system_clock> GetCurrentMovementTime(const vrpn_TRACKERCB& tracker) {
  time_point<system_clock> current_movement_time(
      seconds(tracker.msg_time.tv_sec)
          + microseconds(tracker.msg_time.tv_usec));
  return current_movement_time;
}

void UpdateGestureString(const vrpn_TRACKERCB& tracker, const int snap_theta,
                         const int snap_phi) {
  int time_since_sensor_0_last_movement = 0;
  int time_since_sensor_1_last_movement = 0;
  char currentLetter = GetCurrentLetter(snap_phi, snap_theta);

  if (tracker.sensor == 0) {
    g_last_sensor_0_letter = currentLetter;
    time_since_sensor_1_last_movement = GetMillisecondsSinceTrackerTime(
        tracker, g_sensor_1_last_movement_time);

    /*
     * If sensor 1 reported new movements since the last time it was grouped,
     * and if it moved not too long ago, group its letter with sensor 0's.
     */
    if ((g_sensor_1_last_grouped_movement_time != g_sensor_1_last_movement_time)
        && (time_since_sensor_1_last_movement
            < GESTURE_GROUPING_TIME_MILLISECONDS)) {
      //printf("S0: Grouping with previous S1. Current: %c, last: %c\n", currentLetter, last_sensor_1_letter);
      g_sensor_0_last_grouped_movement_time = GetCurrentMovementTime(tracker);
      g_sensor_1_last_grouped_movement_time = g_sensor_1_last_movement_time;
      MoveHeadToBeginningOfLetterPair();
      g_gesture_string << currentLetter << g_last_sensor_1_letter;
    } else {
      //printf("S0 ELSE. GroupingT=MovementT?: %i, tSS1: %i\n", sensor_1_last_grouped_movement_time == sensor_1_last_movement_time, time_since_sensor_1);
      g_last_sensor_1_letter = '_';
      g_gesture_string << currentLetter << g_last_sensor_1_letter;
    }
  } else {
    g_last_sensor_1_letter = currentLetter;
    time_since_sensor_0_last_movement = GetMillisecondsSinceTrackerTime(
        tracker, g_sensor_0_last_movement_time);

    /*
     * If sensor 0 reported new movements since the last time it was grouped,
     * and if it moved not too long ago, group its letter with sensor 1's.
     */
    if ((g_sensor_0_last_grouped_movement_time != g_sensor_0_last_movement_time)
        && (time_since_sensor_0_last_movement
            < GESTURE_GROUPING_TIME_MILLISECONDS)) {
      //printf("S1: Grouping with previous S0. Current: %c, last: %c\n", currentLetter, last_sensor_0_letter);
      g_sensor_0_last_grouped_movement_time = g_sensor_0_last_movement_time;
      g_sensor_1_last_grouped_movement_time = GetCurrentMovementTime(tracker);
      MoveHeadToBeginningOfLetterPair();
      g_gesture_string << g_last_sensor_0_letter << currentLetter;
    } else {
      //printf("S1 ELSE. GroupingT=MovementT?: %i, tSS0: %i\n", sensor_0_last_grouped_movement_time == sensor_0_last_movement_time, time_since_sensor_0);
      g_last_sensor_0_letter = '_';
      g_gesture_string << g_last_sensor_0_letter << currentLetter;
    }
  }
  g_gesture_string << ".";

  cout << "Gesture: " << g_gesture_string.str() << endl;
  cout << endl;
}

void UpdateTimers(const vrpn_TRACKERCB& tracker) {
  time_point<system_clock> current_movement_time = GetCurrentMovementTime(
      tracker);
  UpdateTimePoint(g_global_last_movement_time, current_movement_time);

  if (tracker.sensor == 0) {
    UpdateTimePoint(g_sensor_0_last_movement_time, current_movement_time);
  } else {
    UpdateTimePoint(g_sensor_1_last_movement_time, current_movement_time);
  }

  //printf("Updated. GLMT: %i, S0LMT: %i, S1LMT: %i\n", GetMillisecondsSinceTrackerTime(tracker, global_last_movement_time), GetMillisecondsSinceTrackerTime(tracker, sensor_0_last_movement_time), GetMillisecondsSinceTrackerTime(tracker, sensor_1_last_movement_time));
}

#define d(i,j) dd[(i) * (m+2) + (j) ]
#define min(x,y) ((x) < (y) ? (x) : (y))
#define min3(a,b,c) ((a)< (b) ? min((a),(c)) : min((b),(c)))
#define min4(a,b,c,d) ((a)< (b) ? min3((a),(c),(d)) : min3((b),(c),(d)))

int DPrint(int* dd, int n, int m) {
  int i, j;
  for (i = 0; i < n + 2; i++) {
    for (j = 0; j < m + 2; j++) {
      printf("%02d ", d(i, j));
    }
    printf("\n");
  }
  printf("\n");
  return 0;
}

int DlDist2(const char *s, const char* t, int n, int m) {
  int *dd, *DA;
  int i, j, cost, k, i1, j1, DB;
  int infinity = n + m;

  DA = (int*) malloc(256 * sizeof(int));
  dd = (int*) malloc((n + 2) * (m + 2) * sizeof(int));

  d(0,0)= infinity;
  for (i = 0; i < n + 1; i++) {
    d(i+1,1)= i;
    d(i+1,0) = infinity;
  }
  for (j = 0; j < m + 1; j++) {
    d(1,j+1)= j;
    d(0,j+1) = infinity;
  }
  //DPrint(dd,n,m);
  for (k = 0; k < 256; k++)
    DA[k] = 0;
  for (i = 1; i < n + 1; i++) {
    DB = 0;
    for (j = 1; j < m + 1; j++) {
      i1 = DA[t[j - 1]];
      j1 = DB;
      cost = ((s[i - 1] == t[j - 1]) ? 0 : 1);
      if (cost == 0)
        DB = j;
      d(i+1,j+1)=
      min4(d(i,j)+cost,
          d(i+1,j) + 1,
          d(i,j+1)+1,
          d(i1,j1) + (i-i1-1) + 1 + (j-j1-1));
    }
    DA[s[i - 1]] = i;
    //dprint(dd,n,m);
  }
  cost = d(n + 1, m + 1);
  free(dd);
  return cost;
}

/* New size must be a multiple of the original string size */
string ExpandString(const string& input_string, int new_size) {
  stringstream output_string;
  int length_multiplier = new_size / input_string.length();

  vector<string> movement_pairs;
  TokenizeString(input_string, movement_pairs, ".");

  for (vector<string>::iterator it = movement_pairs.begin();
      it < movement_pairs.end(); ++it) {
    for (int i = 0; i < length_multiplier; i++) {
      output_string << *it << ".";
    }
  }

  return output_string.str();
}

struct GestureEntry {
  string name;
  string movements;
  int dl_distance;
  float dl_distance_pct;
};

bool GestureEntryLessThan(GestureEntry i, GestureEntry j) {
  return (i.dl_distance_pct < j.dl_distance_pct);
}

int ReadAndCompareGesturesFromFile(vector<GestureEntry>& aGestures) {
  ifstream gesture_file;
  string current_line;

  cout << "Current gesture" << endl << "\t" << g_gesture_string.str() << endl
       << endl;

  gesture_file.open("gestures.dat");
  if (!gesture_file.is_open()) {
    return RECOGNIZER_ERROR;
  }

  while (getline(gesture_file, current_line)) {
    //cout << "Line: " << current_line << endl;
    stringstream ss(current_line);
    const string& inputGesture = g_gesture_string.str();

    GestureEntry new_gesture_entry;
    ss >> new_gesture_entry.name >> new_gesture_entry.movements;

    int gesture_length_least_common_multiple = boost::math::lcm(
        inputGesture.length(), new_gesture_entry.movements.length());
    string expanded_input_string = ExpandString(
        inputGesture, gesture_length_least_common_multiple);
    string expanded_new_gesture_entry_string = ExpandString(
        new_gesture_entry.movements, gesture_length_least_common_multiple);

    new_gesture_entry.dl_distance = DlDist2(
        expanded_input_string.c_str(),
        expanded_new_gesture_entry_string.c_str(),
        expanded_input_string.length(),
        expanded_new_gesture_entry_string.length());
    new_gesture_entry.dl_distance_pct = (new_gesture_entry.dl_distance * 100.0f)
        / expanded_new_gesture_entry_string.length();

    cout << new_gesture_entry.name << endl << "\t"
         << new_gesture_entry.movements << endl;
    cout << "Distance: " << new_gesture_entry.dl_distance_pct << "% ("
         << new_gesture_entry.dl_distance << " D-L ops)" << endl;
    cout << endl;

    aGestures.push_back(new_gesture_entry);
  }

  return RECOGNIZER_NO_ERROR;
}

void DrawMatchingGestures(const GestureEntry& closest_gesture) {
  stringstream viewer_command;
  string viewer_command_prefix =
      "cd ~/lager/viewer/src/ && ../build/lager_viewer --gesture ";
  string hide_output_suffix = " > /dev/null";

  cout << "Drawing input gesture..." << endl;

  viewer_command << viewer_command_prefix << g_gesture_string.str()
                 << hide_output_suffix;
  system(viewer_command.str().c_str());

  cout << "Drawing gesture match..." << endl;

  viewer_command.str("");
  viewer_command << viewer_command_prefix << closest_gesture.movements
                 << hide_output_suffix;
  system(viewer_command.str().c_str());
}

bool IsSingleSensorGesture() {
  vector<string> movement_pairs;
  TokenizeString(g_gesture_string.str(), movement_pairs, ".");
  bool sensor_0_moved = false;
  bool sensor_1_moved = false;

  for (vector<string>::iterator it = movement_pairs.begin();
      it < movement_pairs.end(); ++it) {
    // Check if sensor 0 movement is present
    if ((*it).c_str()[0] != '_') {
      sensor_0_moved = true;
    }

    // Check if sensor 0 movement is present
    if ((*it).c_str()[0] != '_') {
      sensor_1_moved = true;
    }

    // If we already found movement on both sensors, there is no need to keep checking
    if (sensor_0_moved && sensor_1_moved) {
      break;
    }
  }

  return (!sensor_0_moved || !sensor_1_moved);
}

void RecognizeGesture() {
  vector<GestureEntry> gestures;
  int lowestDistance;
  int gesture_distance_threshold_pct =
      IsSingleSensorGesture() ?
      SINGLE_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT :
                                DUAL_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT;

  cout << " ________________________________ " << endl;
  cout << "|                                |" << endl;
  cout << "|         RECOGNIZING...         |" << endl;
  cout << "|________________________________|" << endl;
  cout << "                                  " << endl;

  if (ReadAndCompareGesturesFromFile(gestures) != RECOGNIZER_NO_ERROR) {
    cout << "Error reading gestures from file." << endl;
    return;
  }

  GestureEntry closest_gesture = *min_element(gestures.begin(), gestures.end(),
                                              GestureEntryLessThan);
  cout << endl;
  cout << "Closest gesture:\t" << closest_gesture.name << endl;
  cout << "Distance:\t\t" << closest_gesture.dl_distance_pct << "% ("
       << closest_gesture.dl_distance << " D-L ops)" << endl;
  cout << "Threshold:\t\t" << gesture_distance_threshold_pct << "%" << endl;

  if (closest_gesture.dl_distance_pct <= gesture_distance_threshold_pct) {
    cout << " ________________________________ " << endl;
    cout << "|                                |" << endl;
    cout << "|          MATCH FOUND!          |" << endl;
    cout << "|________________________________|" << endl;
    cout << "                                  " << endl;
    DrawMatchingGestures(closest_gesture);
  } else {
    cout << endl;
    cout << "NO MATCH." << endl;
  }

  cout << endl << endl;
  ;
}

/*****************************************************************************
 *
 Callback handler
 *
 *****************************************************************************/

void VRPN_CALLBACK handle_tracker_change(void *user_data,
                                         const vrpn_TRACKERCB tracker) {
  vrpn_TRACKERCB *last_report;  // keep track of the current sensor's last report
  vrpn_float64 deltaX, deltaY, deltaZ;
  double theta, phi;
  int snap_theta, snap_phi;
  static float dist_interval_sq = DISTANCE_INTERVAL_SQUARED;

  if (tracker.sensor == 0) {
    last_report = &g_last_report_0;
  } else {
    last_report = &g_last_report_1;
  }

  if (GetDistanceSquared(*last_report, tracker) > dist_interval_sq) {
    printf("Update for sensor: %i at time: %ld.%06ld\n", tracker.sensor,
           tracker.msg_time.tv_sec, tracker.msg_time.tv_usec);
    //printf("GLMT: %i, S0LMT: %i, S1LMT: %i\n", GetMillisecondsSinceTrackerTime(tracker, global_last_movement_time), GetMillisecondsSinceTrackerTime(tracker, sensor_0_last_movement_time), GetMillisecondsSinceTrackerTime(tracker, sensor_1_last_movement_time));
    //printSensorCoordinates(last_report, tracker);

    UpdateTimers(tracker);

    CalculateMovementDeltas(tracker, last_report, deltaX, deltaY, deltaZ);
    CalculateMovementAngles(theta, phi, snap_theta, snap_phi, deltaX, deltaY,
                            deltaZ);

    UpdateGestureString(tracker, snap_theta, snap_phi);

    *last_report = tracker;
  }
}

void VRPN_CALLBACK handle_button_change(void *user_data,
                                        const vrpn_BUTTONCB button) {
  static int count = 0;
  static int button_state = 1;
  int done = 0;

  if (button.state != button_state) {
    printf("button #%d is in state %d\n", button.button, button.state);
    button_state = button.state;
    count++;
  }
  if (count > 4)
    done = 1;
  *(int *) user_data = done;
}

void VRPN_CALLBACK dummy_handle_tracker_change(void *user_data,
                                               const vrpn_TRACKERCB tracker) {
  //printf("DUMMY CALLED for sensor %i!\n", tracker.sensor);
  if (tracker.sensor == 0) {
    g_last_report_0 = tracker;
  } else {
    g_last_report_1 = tracker;
  }

  int *num_reports_received = (int *) user_data;
  *num_reports_received = *num_reports_received + 1;
}

int main(int argc, char *argv[]) {
  printf("Generates strings for movement of tracker %s\n\n", SENSOR_SERVER);

  int num_reports_received = 0;
  int done = 0;
  vrpn_Tracker_Remote *tracker;
  vrpn_Button_Remote *button;
  struct timespec sleep_interval = { 0, MAIN_SLEEP_INTERVAL_MICROSECONDS };

  // initialize the tracker
  tracker = new vrpn_Tracker_Remote(SENSOR_SERVER);

  // initialize the button
  button = new vrpn_Button_Remote(SENSOR_SERVER);

  tracker->register_change_handler(&num_reports_received,
                                   dummy_handle_tracker_change);
  while (num_reports_received != 2) {
    nanosleep(&sleep_interval, NULL);
    tracker->mainloop();
  }

  tracker->unregister_change_handler(&num_reports_received,
                                     dummy_handle_tracker_change);
  tracker->register_change_handler(NULL, handle_tracker_change);
  button->register_change_handler(&done, handle_button_change);

  cout << " ________________________________ " << endl;
  cout << "|                                |" << endl;
  cout << "|       COLLECTING DATA...       |" << endl;
  cout << "|________________________________|" << endl;
  cout << "                                  " << endl;

  // Main loop
  while (!done) {

    // Let tracker receive position information from remote tracker
    tracker->mainloop();
    // Let button receive button status from remote button
    button->mainloop();

    // If gesture buildup pauses, attempt to recognize it
    int gesture_string_length = g_gesture_string.str().length();
    if (GesturePaused() && gesture_string_length > 0) {
      /*
       * Only try to recognize gestures with more than one movement pair.
       * This reduces spurious recognitions from inadvertent movements.
       */
      if (gesture_string_length > 3) {
        RecognizeGesture();
      }

      cout << " ________________________________ " << endl;
      cout << "|                                |" << endl;
      cout << "|       COLLECTING DATA...       |" << endl;
      cout << "|________________________________|" << endl;
      cout << "                                  " << endl;

      ResetGestureString();
    }

    // Sleep so we don't take up 100% of CPU
    nanosleep(&sleep_interval, NULL);
  }

} /* main */
