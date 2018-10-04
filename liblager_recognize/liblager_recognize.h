/*
 * liblager_recognize.h
 *
 *  Created on: Mar 2, 2015
 *      Author: andres
 */

#ifndef LIBLAGER_RECOGNIZE_H_
#define LIBLAGER_RECOGNIZE_H_

#include <chrono>
using std::chrono::system_clock;
using std::chrono::time_point;
#include <string>
using std::string;
#include <vector>
using std::vector;

#define RECOGNIZER_ERROR -1
#define RECOGNIZER_NO_ERROR 0

/**
 * Recognizes an input LaGeR gesture by comparing it to a list of subscribed
 * gesture candidates and finding the closest match.
 */
class LagerRecognizer {
 public:
  /**
   * Returns a pointer to an instance of the class.
   * Instantiates the class if needed.
   */
  static LagerRecognizer* Instance(
      vector<struct SubscribedGesture>* subscribed_gestures);

  /**
   * Empty destructor for this class.
   */
  ~LagerRecognizer() {
  }

  /**
   * Takes a gesture LaGeR string and returns the closest matching subscribed
   * gesture.
   *
   * If no match is found below a certain distance threshold, it
   * indicates it by toggling a Boolean parameter.
   *
   * Another Boolean parameter determines whether or not to invoke lager_viewer
   * to draw the input and matching gestures on screen.
   */
  struct SubscribedGesture RecognizeGesture(bool draw_gestures,
                                            string current_gesture,
                                            bool& match_found);

  /**
    * Initializes and returns a Python classifier function.
    */
  PyObject* InitializePythonClassifier();

  /**
   * Takes a gesture LaGeR string, finds the closest matching subscribed
   * gesture via a machine learning algorithm, prints and returns its index.
   *
   * If no match is found above a certain probability threshold, it indicates
   * it by toggling a Boolean parameter.
   */
  long RecognizeGestureML(PyObject* python_classifier,
                          string current_gesture,
                          bool& match_found);

 private:
  /**
   * Private constructor for this class, which takes a pointer to a
   * SubscribedGesture vector and assigns it to a member variable.
   */
  LagerRecognizer(vector<struct SubscribedGesture>* subscribed_gestures)
      : subscribed_gestures_(subscribed_gestures) {
  }
  ;

  /**
   * Empty constructor for this class, with a parameter taking a reference to
   * a class instance.
   */
  LagerRecognizer(LagerRecognizer const&) {
  }
  ;

  /**
   * Empty implementation of the operator= for this class.
   */
  LagerRecognizer& operator=(LagerRecognizer const&) {
  }
  ;

  /**
   * Takes a time point and returns the number of milliseconds that have
   * elapsed until the current time.
   */
  int GetMillisecondsUntilNow(const time_point<system_clock> &last_time);

  /**
   * Takes a LaGeR gesture string and returns whether or not it corresponds to
   * the movement of a single sensor.
   */
  bool IsSingleSensorGesture(string current_gesture);

  /**
   * Takes the closest gesture match, the distance threshold, the recognition
   * starting time, and whether a match was found, then prints the
   * corresponding recognition results.
   */
  void PrintRecognitionResults(struct SubscribedGesture& closest_gesture,
                               int gesture_distance_threshold_pct,
                               time_point<system_clock> recognition_start_time,
                               bool match_found);

  /**
   * Takes a reference to a SubscribedGesture and the LaGeR string of the input
   * gesture being recognized, then updates the SubscribedGesture's distance
   * members.
   *
   */
  void UpdateSubscribedGestureDistance(
      struct SubscribedGesture& subscribed_gesture, string current_gesture);

  /**
   * Takes the LaGeR string of the input gesture being recognized, then
   * iterates through the SubscribedGestures and updates their distance
   * members.
   */
  void UpdateSubscribedGestureDistances(string current_gesture);

  /// Pointer to an instance of this class
  static LagerRecognizer* instance_;

  /// Pointer to vector of SubscribedGestures
  vector<struct SubscribedGesture>* subscribed_gestures_;
};

#endif /* LIBLAGER_RECOGNIZE_H_ */
