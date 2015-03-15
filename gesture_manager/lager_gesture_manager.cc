#include <stdlib.h>     // for atoi
#include <iostream>   // for std::cout
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "liblager_convert.h"

#define GESTURE_MANAGER_ERROR -1
#define GESTURE_MANAGER_NO_ERROR 0

using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;
using std::vector;

/* Globals */

struct GestureEntry {
  string name;
  string lager;
};

int ReadGesturesFromFile(vector<GestureEntry>& gestures, const string& gestures_file_name) {
  ifstream gestures_file;
  string current_line;

  gestures.clear();

  gestures_file.open(gestures_file_name.c_str());
  if (!gestures_file.is_open()) {
    return GESTURE_MANAGER_ERROR;
  }

  while (getline(gestures_file, current_line)) {
    string name, lager;
    stringstream ss(current_line);
    //ss >> name >> lager;

    GestureEntry new_gesture;
    ss >> new_gesture.name >> new_gesture.lager;
    gestures.push_back(new_gesture);
  }

  return GESTURE_MANAGER_NO_ERROR;
}

int WriteGesturesToFile(vector<GestureEntry>& gestures, string& gestures_file_name) {
  ofstream gestures_file;
  string current_line;

  gestures_file.open(gestures_file_name.c_str(), std::ofstream::out | std::ofstream::trunc);
  if (!gestures_file.is_open()) {
    return GESTURE_MANAGER_ERROR;
  }

  for (vector<GestureEntry>::iterator it = gestures.begin();
      it < gestures.end(); ++it) {
    gestures_file << it->name << " " << it->lager << endl;
  }
}

void PrintWelcomeBanner() {
  cout << " ________________________________ " << endl;
  cout << "|                                |" << endl;
  cout << "|     LAGER GESTURE MANAGER      |" << endl;
  cout << "|________________________________|" << endl;
  cout << "                                  " << endl;
}

void PrintOptionsMenu() {
  cout << "Please choose an option:" << endl;
  cout << endl;
  cout << "1. List gestures" << endl;
  cout << "2. Add gesture with lager string" << endl;
  cout << "3. Add gesture with sensor" << endl;
  cout << "4. Show gesture" << endl;
  cout << "5. Edit gesture" << endl;
  cout << "6. Delete gesture" << endl;
  cout << endl;
  cout << "7. Quit" << endl;
  cout << endl;
  cout << "Choice: ";
}

void ListGestures(vector<GestureEntry>& gestures) {
  int gestureNumber = 1;
  for (vector<GestureEntry>::iterator it = gestures.begin();
      it < gestures.end(); ++it) {
    cout << gestureNumber << ": " << it->name << endl;
    gestureNumber++;
  }
}

void AddGestureWithLager(vector<GestureEntry>& gestures) {
  GestureEntry new_gesture;

  cout << "Enter gesture name (no spaces): ";
  getline(cin, new_gesture.name);

  while (new_gesture.name.find(" ") != std::string::npos) {
    cout << "Error: Space detected. Please try again." << endl;
    cout << "Enter gesture name (no spaces): ";
    getline(cin, new_gesture.name);
  }

  cout << "Enter gesture lager: ";
  std::getline(cin, new_gesture.lager);

  gestures.push_back(new_gesture);
}

void AddGestureWithSensor(vector<GestureEntry>& gestures) {
  GestureEntry new_gesture;
  LagerConverter* lager_converter = LagerConverter::Instance();

  cout << "Enter gesture name (no spaces): ";
  getline(cin, new_gesture.name);

  while (new_gesture.name.find(" ") != std::string::npos) {
    cout << "Error: Space detected. Please try again." << endl;
    cout << "Enter gesture name (no spaces): ";
    getline(cin, new_gesture.name);
  }

  cout << "Draw gesture with sensor..." << endl << std::flush;

  lager_converter->SetUseButtons(true);
  lager_converter->Start();

  new_gesture.lager = lager_converter->BlockingGetLagerString();
  delete(lager_converter);
  cout << "Gesture lager: " << new_gesture.lager << endl << endl;

  gestures.push_back(new_gesture);
}

void ShowGesture(vector<GestureEntry>& gestures, int gesture_number) {
  GestureEntry& gesture = gestures[gesture_number - 1];

  cout << "Gesture name: " << gesture.name << endl;
  cout << "Gesture lager: " << gesture.lager << endl;

  stringstream viewer_command;
  string viewer_command_prefix =
      "lager_viewer --gesture ";
  string hide_output_suffix = " > /dev/null";

  cout << "Drawing gesture..." << endl;

  viewer_command << viewer_command_prefix << gesture.lager
                 << hide_output_suffix;

  system(viewer_command.str().c_str());
}

void EditGesture(vector<GestureEntry>& gestures, int gesture_number) {
  GestureEntry& gesture = gestures[gesture_number - 1];
  string input;

  cout << "Old gesture name: " << gesture.name << endl;
  cout << "New gesture name (blank to leave unchanged): ";
  getline(cin, input);
  if (input.compare(string("")) != 0) {
    gesture.name = input;
  }

  cout << "Old gesture lager: " << gesture.lager << endl;
  cout << "New gesture lager (blank to leave unchanged): ";
  getline(cin, input);
  if (input.compare(string("")) != 0) {
    gesture.lager = input;
  }
}

void DeleteGesture(vector<GestureEntry>& gestures, int gesture_number) {
  gestures.erase(gestures.begin() + (gesture_number - 1));
}

int main(int argc, const char *argv[]) {
  vector<GestureEntry> gestures;
  string gestures_file_name;
  string input;
  std::string::size_type sz;   // alias of size_t
  bool quit = false;

  if (argc < 2) {
    cout << "You must enter the name of a gestures file to manage." << endl;
    cout << "Exiting..." << endl;
    exit(1);
  } else {
    gestures_file_name = string(argv[1]);
  }

  ReadGesturesFromFile(gestures, gestures_file_name);
  PrintWelcomeBanner();

  do{
      PrintOptionsMenu();
      getline(cin, input);
      switch (atoi(input.c_str())) {
        case 1:
          cout << endl;
          ListGestures(gestures);
          cout << endl;
          break;
        case 2:
          cout << endl;
          AddGestureWithLager(gestures);
          cout << endl;
          break;
        case 3:
          cout << endl;
          AddGestureWithSensor(gestures);
          cout << endl;
          break;
        case 4:
          cout << "Enter number of gesture to show: ";
          getline(cin, input);
          ShowGesture(gestures, atoi(input.c_str()));
          break;
        case 5:
          cout << "Enter number of gesture to edit: ";
          getline(cin, input);
          EditGesture(gestures, atoi(input.c_str()));
          break;
        case 6:
          cout << "Enter number of gesture to delete: ";
          getline(cin, input);
          DeleteGesture(gestures, atoi(input.c_str()));
          break;
        case 7:
          quit = true;
          WriteGesturesToFile(gestures, gestures_file_name);
          break;
      }
    } while(!quit);

  exit(0);
}
