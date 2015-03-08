// phan_client.C - simplest example: generates a flat horizontal plane

#include <stdio.h>    // for printf, NULL
#include <time.h>     // for nanosleep
#include <algorithm>  // for std::min_element
#include <iostream>   // for std::cout
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>

#include <boost/math/common_factor.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "liblager_connect.h"
#include "liblager_convert.h"

#include "spherical_coordinates.h"
#include "coordinates_letter.h"
#include "string_tokenizer.h"

#define RECOGNIZER_ERROR -1
#define RECOGNIZER_NO_ERROR 0

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
using std::chrono::system_clock;
using std::chrono::time_point;

using namespace boost::interprocess;

/* Globals */

struct SubscribedGesture {
  string name;
  string lager;
  string expanded_lager;
  pid_t pid;
  int dl_distance;
  float dl_distance_pct;
};

string g_gesture_string;

vector<SubscribedGesture> g_subscribed_gestures;

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

bool GestureEntryLessThan(SubscribedGesture i, SubscribedGesture j) {
  return (i.dl_distance_pct < j.dl_distance_pct);
}

void UpdateSubscribedGestureDistance(
    struct SubscribedGesture& subscribed_gesture) {
  const string input_gesture = g_gesture_string;

  int gesture_length_least_common_multiple = boost::math::lcm(
      input_gesture.length(), subscribed_gesture.lager.length());

  string expanded_input_string = ExpandString(
      input_gesture, gesture_length_least_common_multiple);
  subscribed_gesture.expanded_lager = ExpandString(
      subscribed_gesture.lager, gesture_length_least_common_multiple);

  subscribed_gesture.dl_distance = DlDist2(
      expanded_input_string.c_str(), subscribed_gesture.expanded_lager.c_str(),
      expanded_input_string.length(),
      subscribed_gesture.expanded_lager.length());

  subscribed_gesture.dl_distance_pct = (subscribed_gesture.dl_distance * 100.0f)
      / subscribed_gesture.expanded_lager.length();

  cout << subscribed_gesture.name << endl << "\t" << subscribed_gesture.lager
       << endl;
  cout << "Distance: " << subscribed_gesture.dl_distance_pct << "% ("
       << subscribed_gesture.dl_distance << " D-L ops)" << endl;
  cout << endl;
}

void UpdateSubscribedGestureDistances() {
  for (vector<SubscribedGesture>::iterator it = g_subscribed_gestures.begin();
      it < g_subscribed_gestures.end(); ++it) {
    UpdateSubscribedGestureDistance(*it);
  }
}

void DrawMatchingGestures(const SubscribedGesture& closest_gesture) {
  stringstream viewer_command;
  string viewer_command_prefix =
      "cd ~/lager/viewer/src/ && ../build/lager_viewer --gesture ";
  string hide_output_suffix = " > /dev/null";

  cout << "Drawing input gesture..." << endl;

  viewer_command << viewer_command_prefix << g_gesture_string
                 << hide_output_suffix;
  system(viewer_command.str().c_str());

  cout << "Drawing gesture match..." << endl;

  viewer_command.str("");
  viewer_command << viewer_command_prefix << closest_gesture.lager
                 << hide_output_suffix;
  system(viewer_command.str().c_str());
}

bool IsSingleSensorGesture() {
  vector<string> movement_pairs;
  TokenizeString(g_gesture_string, movement_pairs, ".");
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

int GetMillisecondsUntilNow(const time_point<system_clock> &last_time) {
  time_point<system_clock> now = system_clock::now();
  return duration<double, std::milli>(now - last_time).count();
}

void RecognizeGesture(bool use_gestures_file, bool draw_gestures) {
  if (g_subscribed_gestures.size() == 0) {
    cout << "No gesture subscriptions." << endl;
    return;
  }

  int lowestDistance;
  bool match_found = false;
  unsigned int num_milliseconds_since_recognition_start = 0;
  time_point<system_clock> recognition_start_time = system_clock::now();
  int gesture_distance_threshold_pct =
      IsSingleSensorGesture() ?
      SINGLE_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT :
                                DUAL_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT;

  cout << " ________________________________ " << endl;
  cout << "|                                |" << endl;
  cout << "|         RECOGNIZING...         |" << endl;
  cout << "|________________________________|" << endl;
  cout << "                                  " << endl;

  UpdateSubscribedGestureDistances();

  SubscribedGesture closest_gesture = *min_element(
      g_subscribed_gestures.begin(), g_subscribed_gestures.end(),
      GestureEntryLessThan);
  match_found = closest_gesture.dl_distance_pct
      <= gesture_distance_threshold_pct;
  num_milliseconds_since_recognition_start = GetMillisecondsUntilNow(
      recognition_start_time);

  if (match_found) {
    cout << " ________________________________ " << endl;
    cout << "|                                |" << endl;
    cout << "|          MATCH FOUND!          |" << endl;
    cout << "|________________________________|" << endl;
    cout << "                                  " << endl;
  } else {
    cout << endl;
    cout << "NO MATCH." << endl;
  }

  cout << endl;
  cout << "Closest gesture:\t" << closest_gesture.name << endl;
  cout << endl;
  cout << "Distance:\t\t" << closest_gesture.dl_distance_pct << "% ("
       << closest_gesture.dl_distance << " D-L ops)" << endl;
  cout << "Threshold:\t\t" << gesture_distance_threshold_pct << "%" << endl;
  cout << endl;
  cout << "Recognition time: \t" << num_milliseconds_since_recognition_start
       << "ms" << endl;
  cout << endl;

  if (match_found) {
    if (draw_gestures) {
      DrawMatchingGestures(closest_gesture);
    }
    if (!use_gestures_file && closest_gesture.pid != 0) {
      SendDetectedGestureMessage(closest_gesture.name, closest_gesture.pid);
    }
  }

  cout << endl << endl;
  ;
}

/*****************************************************************************
 *
 Callback handler
 *
 *****************************************************************************/

bool DetermineButtonUse(const int argc, const char** argv) {
  bool use_buttons;

  if ((argc > 1)
      && (std::string(argv[1]).find("--use_buttons") != std::string::npos)) {
    cout << "Gesture detection will be active while pressing a button." << endl;
    use_buttons = true;
  } else {
    cout << "Gesture detection will be active at all times." << endl;
    use_buttons = false;
  }

  return use_buttons;
}

bool DetermineGesturesFileUse(const int argc, const char** argv) {
  bool use_gestures_file;

  if ((argc > 1)
      && (std::string(argv[1]).find("--use_gestures_file") != std::string::npos)) {
    cout << "Comparing input to gestures in a file." << endl;
    use_gestures_file = true;
  } else {
    cout << "Comparing input to subscribed gestures." << endl;
    use_gestures_file = false;
  }

  return use_gestures_file;
}

bool DetermineGestureDrawing(const int argc, const char** argv) {
  bool draw_gestures;

  if ((argc > 1)
      && (std::string(argv[1]).find("--draw_gestures") != std::string::npos)) {
    cout << "Using lager_viewer to draw input and subscribed gestures." << endl;
    draw_gestures = true;
  } else {
    cout << "Will not draw gestures." << endl;
    draw_gestures = false;
  }

  return draw_gestures;
}

void AddSubscribedGestures() {
  while (true) {
    GestureSubscriptionMessage message = GetGestureSubscriptionMessage();

    cout << "Adding subscription..." << endl;
    cout << endl;
    cout << "Name: \"" << message.gesture_name() << "\"" << endl;
    cout << "Lager: \"" << message.gesture_lager() << endl;
    cout << endl;

    SubscribedGesture subscription;
    subscription.name = message.gesture_name();
    subscription.lager = message.gesture_lager();
    subscription.pid = message.pid();
    g_subscribed_gestures.push_back(subscription);
  }
}

int GetSubscribedGesturesFromFile() {
  ifstream gestures_file;
  string current_line;

  cout << "Current gesture" << endl << "\t" << g_gesture_string << endl
       << endl;

  gestures_file.open("gestures.dat");
  if (!gestures_file.is_open()) {
    return RECOGNIZER_ERROR;
  }

  while (getline(gestures_file, current_line)) {
    string name, lager;
    stringstream ss(current_line);
    ss >> name >> lager;

    SubscribedGesture new_gesture;
    ss >> new_gesture.name >> new_gesture.lager;
    new_gesture.pid = 0;
    g_subscribed_gestures.push_back(new_gesture);
  }

  return RECOGNIZER_NO_ERROR;
}

int main(int argc, const char *argv[]) {
  //printf("Generates strings for movement of tracker %s\n\n", TRACKER_SERVER);

  struct timespec sleep_interval = { 0, MAIN_SLEEP_INTERVAL_MICROSECONDS };
  bool use_buttons = DetermineButtonUse(argc, argv);
  bool use_gestures_file = DetermineGesturesFileUse(argc, argv);
  bool draw_gestures = DetermineGestureDrawing(argc, argv);
  LagerConverter* lager_converter = LagerConverter::Instance();

  if (use_gestures_file) {
    GetSubscribedGesturesFromFile();
  } else {
    CreateGestureSubscriptionQueue();
    boost::thread subscription_updater(AddSubscribedGestures);
  }

  cout << " ________________________________ " << endl;
  cout << "|                                |" << endl;
  cout << "|       COLLECTING DATA...       |" << endl;
  cout << "|________________________________|" << endl;
  cout << "                                  " << endl;

  lager_converter->SetUseButtons(use_buttons);
  lager_converter->Start();

  while(true) {
    g_gesture_string = lager_converter->BlockingGetLagerString();
    RecognizeGesture(use_gestures_file, draw_gestures);
  }
} /* main */
