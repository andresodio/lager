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

class LagerRecognizer {
 public:
  static LagerRecognizer* Instance(
      vector<struct SubscribedGesture>* subscribed_gestures);

  ~LagerRecognizer() {
  }

  struct SubscribedGesture RecognizeGesture(bool draw_gestures,
                                            string current_gesture,
                                            bool& match_found);

 private:
  LagerRecognizer(vector<struct SubscribedGesture>* subscribed_gestures)
      : subscribed_gestures_(subscribed_gestures) {
  }
  ;
  LagerRecognizer(LagerRecognizer const&) {
  }
  ;
  LagerRecognizer& operator=(LagerRecognizer const&) {
  }
  ;

  int GetMillisecondsUntilNow(const time_point<system_clock> &last_time);
  bool IsSingleSensorGesture(string current_gesture);
  void PrintRecognitionResults(struct SubscribedGesture& closest_gesture,
                               int gesture_distance_threshold_pct,
                               time_point<system_clock> recognition_start_time,
                               bool match_found);
  void UpdateSubscribedGestureDistance(
      struct SubscribedGesture& subscribed_gesture, string current_gesture);
  void UpdateSubscribedGestureDistances(string current_gesture);

  static LagerRecognizer* instance_;
  vector<struct SubscribedGesture>* subscribed_gestures_;
};

#endif /* LIBLAGER_RECOGNIZE_H_ */
