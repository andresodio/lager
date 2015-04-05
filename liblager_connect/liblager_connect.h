#ifndef LAGER_LIBLAGER_CONNECT_LIBLAGER_CONNECT_H
#define LAGER_LIBLAGER_CONNECT_LIBLAGER_CONNECT_H

#include <sys/types.h>
#include <unistd.h>
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/thread/thread.hpp>

#define MAX_DETECTED_GESTURE_MSG_SIZE 1000

/**
 * Describes the structure of a subscribed gesture
 */
struct SubscribedGesture {
  string name;
  string lager;
  string expanded_lager;
  pid_t pid;
  int distance;
  float distance_pct;
};

/**
 * Encodes and serializes detected gesture messages going from the recognizer
 * to its subscribers.
 *
 * Code structure based on http://stackoverflow.com/a/12349823
 */
class DetectedGestureMessage {
 public:
  /**
   * Constructor which takes a gesture name.
   */
  DetectedGestureMessage(string gesture_name = "")
      : gesture_name_(gesture_name) {
  }
  ;

  /**
   * Returns the name of the gesture in the message.
   */
  string get_gesture_name() const {
    return gesture_name_;
  }
  ;

 private:
  friend class boost::serialization::access;

  string gesture_name_;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & gesture_name_;
  }
};

/*
 * Encodes and serializes gesture subscription messages going from the
 * subscribers to the recognizer.
 *
 * Code structure based on http://stackoverflow.com/a/12349823
 */
class GestureSubscriptionMessage {
 public:
  /**
   * Constructor which takes a PID, gesture name, and gesture lager
   * representation for the subscription message.
   *
   * The PID identifies the process that is requesting the subscription.
   */
  GestureSubscriptionMessage(pid_t pid = 0, string gesture_name = "",
                             string gesture_lager = "")
      : pid_(pid),
        gesture_name_(gesture_name),
        gesture_lager_(gesture_lager) {
  }
  ;

  /**
   * Returns the name of the gesture in the subscription message.
   */
  string gesture_name() const {
    return gesture_name_;
  }
  /**
   * Returns the LaGeR of the gesture in the subscription message.
   */
  string gesture_lager() const {
    return gesture_lager_;
  }
  /**
   * Returns the PID of the process that is requesting the subscription.
   */
  pid_t pid() const {
    return pid_;
  }

 private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & pid_;
    ar & gesture_name_;
    ar & gesture_lager_;
  }

  string gesture_name_;
  string gesture_lager_;
  pid_t pid_;

};

/**
 * Creates a message queue for subscribing to detected gesture notifications.
 */
void CreateGestureSubscriptionQueue();
/**
 * Constantly monitors the message queue and adds new subscriptions to a
 * vector.
 */
void AddSubscribedGestures(vector<SubscribedGesture>* subscribed_gestures);
/**
 * Blocking function that gets and returns a subscription message from the
 * queue.
 *
 * Code structure based on http://stackoverflow.com/a/12349823
 */
GestureSubscriptionMessage GetGestureSubscriptionMessage();
void SendGestureSubscriptionMessage(string gesture_name, string gesture_lager);
void SubscribeToGesturesInFile(string file_name);

DetectedGestureMessage GetDetectedGestureMessage();
void SendDetectedGestureMessage(string gesture_name, pid_t destination_pid);

#endif /* LAGER_LIBLAGER_CONNECT_LIBLAGER_CONNECT_H */
