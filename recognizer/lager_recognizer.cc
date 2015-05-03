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
 * Reads the program arguments and returns whether sensor buttons are going to
 * be used to start/stop gesture detection, or whether buttons will be ignored
 * and detection will be active at all times.
 */
bool DetermineButtonUse(const int argc, const char** argv) {
  bool use_buttons;

  if ((argc > 1)
      && (std::string(argv[1]).find("--no_buttons") != std::string::npos)) {
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

/**
 * Reads the program arguments and returns whether or not the input and
 * subscribed gestures will be drawn on screen via lager_viewer.
 */
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
    ss >> name >> lager;

    SubscribedGesture new_gesture;
    ss >> new_gesture.name >> new_gesture.lager;
    new_gesture.pid = 0;
    g_subscribed_gestures.push_back(new_gesture);
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

  cout << "Drawing gesture match..." << endl;

  viewer_command.str("");
  viewer_command << viewer_command_prefix << closest_gesture.lager
                 << hide_output_suffix;
  system(viewer_command.str().c_str());
}

/**
 * The main loop of the LaGeR Recognizer.
 */
int main(int argc, const char *argv[]) {
  bool use_buttons = DetermineButtonUse(argc, argv);
  bool use_gestures_file = DetermineGesturesFileUse(argc, argv);
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

  lager_converter->SetUseButtons(use_buttons);
  lager_converter->Start();

  while(true) {
    string gesture_string = lager_converter->BlockingGetLagerString();
    if (g_subscribed_gestures.size() > 0) {
        SubscribedGesture recognized_gesture =
            lager_recognizer->RecognizeGesture(draw_gestures, gesture_string, match_found);

        if (draw_gestures) {
                    DrawMatchingGestures(recognized_gesture, gesture_string);
                  }
        if (match_found) {
          if (draw_gestures) {
           // DrawMatchingGestures(recognized_gesture, gesture_string);
          }
          if (!use_gestures_file && recognized_gesture.pid != 0) {
            SendDetectedGestureMessage(recognized_gesture.name, recognized_gesture.pid);
          }
        }
    }
  }
} /* main */
