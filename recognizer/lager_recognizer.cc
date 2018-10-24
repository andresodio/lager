#include <boost/thread/thread.hpp>
#include <iostream>
using std::cout;
using std::endl;
#include <string>
using std::string;
#include <fstream>
using std::ifstream;
#include <sstream>
using std::stringstream;
#include <vector>
using std::vector;

#include <Python.h>

#include <signal.h>

#include "liblager_connect.h"
#include "liblager_convert.h"
#include "liblager_recognize.h"

/* Globals */

/// Global vector of SubscribedGestures
vector<SubscribedGesture> g_subscribed_gestures;

/*****************************************************************************
 *
 Callback handler
 *
 *****************************************************************************/

/**
 * Reads the program arguments and returns whether a given string is present
 */
bool DetermineArgumentPresent(const int argc, const char** argv,
                              const char* string_to_find) {
  bool string_found = false;

  if (argc > 1) {
    int i = 1;
    for (; i < argc; i++) {
      string_found = std::string(argv[i]).find(string_to_find)
          != std::string::npos;
      if (string_found) {
        break;
      }
    }
  }

  return string_found;
}

/**
 * Reads the program arguments and returns whether sensor tracker reports are
 * going to be interpreted as absolute coordinates or as relative movements.
 *
 * If no mode is specified, absolute tracking is used by default.
 */
LCTrackingMode DetermineTrackingMode(const int argc, const char** argv) {
  LCTrackingMode tracking_mode;

  if (DetermineArgumentPresent(argc, argv, "--relative_tracking")) {
    cout << "Sensor tracker reports will be interpreted as relative movements." << endl;
    tracking_mode = LCTrackingMode::relative;
  } else {
    cout << "Sensor tracker reports will be interpreted as absolute coordinates." << endl;
    tracking_mode = LCTrackingMode::absolute;
  }

  return tracking_mode;
}

/**
 * Reads the program arguments and returns whether sensor buttons are going to
 * be used to start/stop gesture detection, or whether buttons will be ignored
 * and detection will be active at all times.
 */
bool DetermineButtonUse(const int argc, const char** argv) {
  bool use_buttons;

  if (DetermineArgumentPresent(argc, argv, "--no_buttons")) {
    cout << "Gesture detection will be active at all times." << endl;
    use_buttons = false;
  } else {
    cout << "Gesture detection will be active while pressing a button." << endl;
    use_buttons = true;
  }

  return use_buttons;
}

/**
 * Reads the program arguments and returns whether the gesture input is going
 * to be compared to subscribed gestures or to a fixed list of gestures in a
 * file.
 */
bool DetermineGesturesFileUse(const int argc, const char** argv) {
  bool use_gestures_file;

  if (DetermineArgumentPresent(argc, argv, "--use_gestures_file")) {
    cout << "Comparing input to gestures in a file." << endl;
    use_gestures_file = true;
  } else {
    cout << "Comparing input to subscribed gestures." << endl;
    use_gestures_file = false;
  }

  return use_gestures_file;
}

/**
 * Reads the program arguments and returns whether LaGeR string updates are
 * going to be printed out as they occur.
 */
bool DetermineUpdatePrinting(const int argc, const char** argv) {
  bool print_updates;

  if (DetermineArgumentPresent(argc, argv, "--print_updates")) {
    cout << "LaGeR string updates will be printed out as they occur." << endl;
    print_updates = true;
  } else {
    cout << "LaGeR string updates will not be printed out." << endl;
    print_updates = false;
  }

  return print_updates;
}

/**
 * Reads the program arguments and returns whether or not the input and
 * subscribed gestures will be drawn on screen via lager_viewer.
 */
bool DetermineGestureDrawing(const int argc, const char** argv) {
  bool draw_gestures;

  if (DetermineArgumentPresent(argc, argv, "--draw_gestures")) {
    cout << "Using lager_viewer to draw input and subscribed gestures." << endl;
    draw_gestures = true;
  } else {
    cout << "Will not draw gestures." << endl;
    draw_gestures = false;
  }

  return draw_gestures;
}

/**
 * Reads a file named gestures.dat and parses it to save its gestures into the
 * global vector of SubscribedGestures.
 */
int GetSubscribedGesturesFromFile() {
  ifstream gestures_file;
  string current_line;

  gestures_file.open("gestures.dat");
  if (!gestures_file.is_open()) {
    return RECOGNIZER_ERROR;
  }

  while (getline(gestures_file, current_line)) {
    string name, lager;
    stringstream ss(current_line);

    SubscribedGesture new_gesture;
    ss >> new_gesture.name >> new_gesture.lager;
    new_gesture.pid = 0;
    g_subscribed_gestures.push_back(new_gesture);
    cout << "Added Name: " << new_gesture.name << " Lager: " << new_gesture.lager << endl;
  }

  return RECOGNIZER_NO_ERROR;
}

/**
 * Takes a reference to a SubscribedGesture and an input gesture LaGeR string,
 * and draws them on screen via lager_viewer.
 */
void DrawMatchingGestures(const SubscribedGesture& closest_gesture, string gesture_string) {
  stringstream viewer_command;
  string viewer_command_prefix =
      "lager_viewer --gesture ";
  string hide_output_suffix = " > /dev/null";

  cout << "Drawing input gesture..." << endl;

  viewer_command << viewer_command_prefix << gesture_string
                 << hide_output_suffix;
  system(viewer_command.str().c_str());

  /*
   * Does not apply to machine learning-based recognition since there is
   * no canonical representation of the closest gesture match.
   *
  cout << "Drawing gesture match..." << endl;

  viewer_command.str("");
  viewer_command << viewer_command_prefix << closest_gesture.lager
                 << hide_output_suffix;
  system(viewer_command.str().c_str());
  */
}

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    /* Reset handler to catch SIGINT next time.
       Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler);
    printf("\n");
    fflush(stdout);
    exit(0);
}

/**
 * The main loop of the LaGeR Recognizer.
 */
int main(int argc, const char *argv[]) {
  /* Set the SIGINT (Ctrl-C) signal handler to sigintHandler
     Refer http://en.cppreference.com/w/c/program/signal */
  signal(SIGINT, sigintHandler);

  LCTrackingMode tracking_mode = DetermineTrackingMode(argc, argv);
  bool use_buttons = DetermineButtonUse(argc, argv);
  bool use_gestures_file = DetermineGesturesFileUse(argc, argv);
  bool print_updates = DetermineUpdatePrinting(argc, argv);
  bool draw_gestures = DetermineGestureDrawing(argc, argv);
  bool match_found = false;
  LagerConverter* lager_converter = LagerConverter::Instance();
  LagerRecognizer* lager_recognizer = LagerRecognizer::Instance(&g_subscribed_gestures);

  if (use_gestures_file) {
    GetSubscribedGesturesFromFile();
  } else {
    CreateGestureSubscriptionQueue();
    boost::thread subscription_updater(AddSubscribedGestures, &g_subscribed_gestures);
  }

  cout << " ________________________________ " << endl;
  cout << "|                                |" << endl;
  cout << "|       COLLECTING DATA...       |" << endl;
  cout << "|________________________________|" << endl;
  cout << "                                  " << endl;

  lager_converter->SetPrintUpdates(print_updates);
  lager_converter->SetTrackingMode(tracking_mode);
  lager_converter->SetUseButtons(use_buttons);
  lager_converter->Start();



  while(true) {
    string gesture_string = lager_converter->BlockingGetLagerString();
    if (g_subscribed_gestures.size() > 0) {

      cout << " ________________________________ " << endl;
      cout << "|                                |" << endl;
      cout << "|          INPUT LAGER           |" << endl;
      cout << "|________________________________|" << endl;
      cout << "                                  " << endl;

      cout << gesture_string << endl << endl;

      lager_recognizer->RecognizeGesture(
          draw_gestures, gesture_string, match_found);

      SubscribedGesture recognized_gesture = lager_recognizer->RecognizeGestureML(
          gesture_string, match_found);

      if (!match_found) {
        continue;
      }

      if (draw_gestures) {
        DrawMatchingGestures(recognized_gesture, gesture_string);
      }

      if (!use_gestures_file && recognized_gesture.pid != 0) {
        cout << "Sending detected gesture" << endl;
        SendDetectedGestureMessage(recognized_gesture.name,
                                    recognized_gesture.pid);
      }
    }
  }
} /* main */
