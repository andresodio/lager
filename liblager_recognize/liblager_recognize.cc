/*
 * liblager_recognize.cc
 *
 *  Created on: Mar 2, 2015
 *      Author: Andrés Odio
 */

#include <algorithm>  // for std::min_element
using std::min_element;
#include <boost/math/common_factor.hpp>
#include <chrono>
using std::chrono::duration;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::chrono::time_point;
#include <iostream>
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::fixed;
#include <iomanip>
using std::setprecision;
#include <Python.h>

#include "liblager_connect.h"
#include "liblager_recognize.h"
#include "string_tokenizer.h"

LagerRecognizer* LagerRecognizer::instance_ = NULL;

LagerRecognizer* LagerRecognizer::Instance(vector<struct SubscribedGesture>* subscribed_gestures) {
  if (!instance_) {
    instance_ = new LagerRecognizer(subscribed_gestures);
  }

  return instance_;
}

#define d(i,j) dd[(i) * (m+2) + (j) ]
#define min(x,y) ((x) < (y) ? (x) : (y))
#define min3(a,b,c) ((a)< (b) ? min((a),(c)) : min((b),(c)))
#define min4(a,b,c,d) ((a)< (b) ? min3((a),(c),(d)) : min3((b),(c),(d)))

/* Calculates the Damerau-Levenshtein distance between two strings.
 * Based on implementation at: http://stackoverflow.com/a/10741694
 */
int DLDistance(const char *s, const char* t, int n, int m) {
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
  return (i.distance_pct < j.distance_pct);
}

void LagerRecognizer::UpdateSubscribedGestureDistance(
    struct SubscribedGesture& subscribed_gesture, string current_gesture) {
  const string input_gesture = current_gesture;

  int gesture_length_least_common_multiple = boost::math::lcm(
      input_gesture.length(), subscribed_gesture.lager.length());

  string expanded_input_string = ExpandString(
      input_gesture, gesture_length_least_common_multiple);
  subscribed_gesture.expanded_lager = ExpandString(
      subscribed_gesture.lager, gesture_length_least_common_multiple);

  subscribed_gesture.distance = DLDistance(
      expanded_input_string.c_str(), subscribed_gesture.expanded_lager.c_str(),
      expanded_input_string.length(),
      subscribed_gesture.expanded_lager.length());

  subscribed_gesture.distance_pct = (subscribed_gesture.distance * 100.0f)
      / subscribed_gesture.expanded_lager.length();
}

void LagerRecognizer::UpdateSubscribedGestureDistances(string current_gesture) {
  for (vector<SubscribedGesture>::iterator it = subscribed_gestures_->begin();
      it < subscribed_gestures_->end(); ++it) {
    UpdateSubscribedGestureDistance(*it, current_gesture);
  }
}

bool LagerRecognizer::IsSingleSensorGesture(string current_gesture) {
  vector<string> movement_pairs;
  TokenizeString(current_gesture, movement_pairs, ".");
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

int LagerRecognizer::GetMillisecondsUntilNow(
    const time_point<system_clock> &last_time) {
  time_point<system_clock> now = system_clock::now();
  return duration<double, std::milli>(now - last_time).count();
}

void LagerRecognizer::PrintRecognitionResults(
    struct SubscribedGesture& closest_gesture,
    int gesture_distance_threshold_pct,
    time_point<system_clock> recognition_start_time, bool match_found) {
  unsigned int num_milliseconds_since_recognition_start =
      GetMillisecondsUntilNow(recognition_start_time);

  cout << "Distances" << endl;
  cout << "---------" << endl;

  for (vector<SubscribedGesture>::iterator it = subscribed_gestures_->begin();
       it < subscribed_gestures_->end(); ++it) {
    cout << "  "
         << std::left << std::setw(15)
         << it->name << " : "
         << setprecision (2) << fixed
         << it->distance_pct << "% ("
         << it->distance << " D-L ops)" << endl;
  }

  if (match_found) {
    cout << " ________________________________ " << endl;
    cout << "|                                |" << endl;
    cout << "|      DAMERAU-LEVENSHTEIN       |" << endl;
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
  cout << "Distance:\t\t" << closest_gesture.distance_pct << "% ("
      << closest_gesture.distance << " D-L ops)" << endl;
  cout << "Threshold:\t\t" << gesture_distance_threshold_pct << "%" << endl;
  cout << endl;
  cout << "Recognition time: \t" << num_milliseconds_since_recognition_start
       << "ms" << endl;
  cout << endl << endl << endl;
}

void LagerRecognizer::PrintMlRecognitionResults(
    struct SubscribedGesture& closest_gesture,
    long recognition_probability,
    int gesture_probability_threshold_pct,
    long recognition_time,
    bool match_found) {

  if (match_found) {
    cout << " ________________________________ " << endl;
    cout << "|                                |" << endl;
    cout << "|         DEEP LEARNING          |" << endl;
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
  cout << "Probability:\t\t" << recognition_probability << "%" << endl;
  cout << "Threshold:\t\t" << gesture_probability_threshold_pct << "%" << endl;
  cout << endl;
  cout << "Recognition time: \t" << recognition_time << "ms" << endl;
  cout << endl << endl << endl;
}

struct SubscribedGesture LagerRecognizer::RecognizeGesture(
    bool draw_gestures, string current_gesture,
    bool& match_found) {
  int lowestDistance;

  cout << " ________________________________ " << endl;
  cout << "|                                |" << endl;
  cout << "|      DAMERAU-LEVENSHTEIN       |" << endl;
  cout << "|          RECOGNIZER            |" << endl;
  cout << "|________________________________|" << endl;
  cout << "                                  " << endl;

  time_point<system_clock> recognition_start_time = system_clock::now();
  int gesture_distance_threshold_pct =
      IsSingleSensorGesture(current_gesture) ?
          SINGLE_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT :
          DUAL_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT;

  UpdateSubscribedGestureDistances(current_gesture);

  SubscribedGesture closest_gesture = *min_element(subscribed_gestures_->begin(),
                                                   subscribed_gestures_->end(),
                                                   GestureEntryLessThan);
  match_found = closest_gesture.distance_pct <= gesture_distance_threshold_pct;

  PrintRecognitionResults(closest_gesture, gesture_distance_threshold_pct,
                          recognition_start_time, match_found);

  return closest_gesture;
}

struct SubscribedGesture LagerRecognizer::RecognizeGestureML(
    string current_gesture,
    bool& match_found) {

  cout << " ________________________________ " << endl;
  cout << "|                                |" << endl;
  cout << "|         DEEP LEARNING          |" << endl;
  cout << "|           RECOGNIZER           |" << endl;
  cout << "|________________________________|" << endl;
  cout << "                                  " << endl;

  struct PythonClassifierResult result = CallPythonClassifier(ml_classifier_, current_gesture);

  match_found = ((result.gesture_index >= 0) && (result.probability > ML_RECOGNITION_THRESHOLD_PCT));

  if (result.gesture_index < 0) {
    cout << "Error returned from ML classifier. Gesture index: " << result.gesture_index;
    result.gesture_index = 0;
  }

  SubscribedGesture recognized_gesture = (*subscribed_gestures_)[result.gesture_index];

  PrintMlRecognitionResults(recognized_gesture, result.probability, ML_RECOGNITION_THRESHOLD_PCT, result.elapsed_time, match_found);

  return recognized_gesture;
}

PyObject* LagerRecognizer::InitializePythonClassifier() {
  PyObject *pName, *pModule, *pFunc;

  string python_module_name = "ml_recognizer";
  string python_function_name = "main";

  Py_Initialize();
  pName = PyUnicode_DecodeFSDefault(python_module_name.c_str());
  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if (pModule != NULL) {
    pFunc = PyObject_GetAttrString(pModule, python_function_name.c_str());
    /* pFunc is a new reference */

    int num_args = 0;
    if (pFunc && PyCallable_Check(pFunc)) {
      return pFunc;
    }
    else {
      if (PyErr_Occurred())
          PyErr_Print();
      fprintf(stderr, "Cannot find function \"%s\"\n", python_function_name.c_str());
      return NULL;
    }
  }
  else {
    PyErr_Print();
    fprintf(stderr, "Failed to load \"%s\"\n", python_module_name.c_str());
    return NULL;
  }

}

struct PythonClassifierResult LagerRecognizer::CallPythonClassifier(
    PyObject* python_classifier,
    string current_gesture) {

  PyObject *pArgs, *pValue;
  PyObject *pReturnValue;
  struct PythonClassifierResult result;
  result.gesture_index = -1;
  result.probability = 0.0;

  if (python_classifier && PyCallable_Check(python_classifier)) {
      pArgs = PyTuple_New(1);

      pValue = PyUnicode_FromString(current_gesture.c_str());
      if (!pValue) {
          Py_DECREF(pArgs);
          fprintf(stderr, "Cannot convert argument\n");
          return result;
      }
      /* pValue reference stolen here: */
      PyTuple_SetItem(pArgs, 0, pValue);

      pValue = PyObject_CallObject(python_classifier, pArgs);
      Py_DECREF(pArgs);

      if ((pValue != NULL) && PyTuple_Check(pValue) && (PyTuple_Size(pValue) == 3)){
          result.gesture_index = PyLong_AsLong(PyTuple_GetItem(pValue, 0));
          result.probability = PyFloat_AsDouble(PyTuple_GetItem(pValue, 1));
          result.elapsed_time = PyLong_AsLong(PyTuple_GetItem(pValue, 2));
          Py_DECREF(pValue);
      }
      else {
          PyErr_Print();
          fprintf(stderr,"Call failed\n");
      }
  }
  else {
      if (PyErr_Occurred())
          PyErr_Print();
      fprintf(stderr, "Cannot find function \n");
  }

  return result;
}
