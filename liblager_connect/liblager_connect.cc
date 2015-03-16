#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "liblager_connect.h"

using namespace boost::interprocess;
using std::cout;
using std::endl;
using std::getline;
using std::ifstream;
using std::ostringstream;
using std::string;
using std::stringstream;

#define MAX_NUM_MSG 100

void CreateGestureSubscriptionQueue() {
  try {
    //Erase previous message queue
    message_queue::remove("gesture_subscription");

    message_queue mq(create_only, "gesture_subscription",
                     MAX_NUM_MSG, MAX_DETECTED_GESTURE_MSG_SIZE);
  } catch (interprocess_exception &ex) {
    std::cerr << ex.what() << std::endl;
  }
}

/* Code structure taken from http://stackoverflow.com/a/12349823 */
GestureSubscriptionMessage GetGestureSubscriptionMessage() {
  GestureSubscriptionMessage message;

  try {
    string queue_name = "gesture_subscription";

    message_queue mq(open_or_create, queue_name.c_str(),
                     MAX_NUM_MSG, MAX_DETECTED_GESTURE_MSG_SIZE);

    message_queue::size_type received_size;
    unsigned int priority;

    stringstream input_stringstream;
    string serialized_string;
    serialized_string.resize(MAX_DETECTED_GESTURE_MSG_SIZE);
    mq.receive(&serialized_string[0], MAX_DETECTED_GESTURE_MSG_SIZE,
               received_size, priority);
    input_stringstream << serialized_string;

    boost::archive::text_iarchive ia(input_stringstream);
    ia >> message;

  } catch (interprocess_exception &ex) {
    std::cerr << ex.what() << endl;
  }

  return message;
}

void SendGestureSubscriptionMessage(string gesture_name, string gesture_lager) {
  try {
    GestureSubscriptionMessage message(getpid(), gesture_name, gesture_lager);
    string queue_name = "gesture_subscription";
    stringstream output_stringstream;

    message_queue mq(open_or_create, queue_name.c_str(),
                     MAX_NUM_MSG, MAX_DETECTED_GESTURE_MSG_SIZE);

    boost::archive::text_oarchive output_archive(output_stringstream);
    output_archive << message;

    std::string serialized_string(output_stringstream.str());
    mq.send(serialized_string.data(), serialized_string.size(), 0);
  } catch (interprocess_exception &ex) {
    std::cerr << ex.what() << endl;
  }
}

void SubscribeToGesturesInFile(string file_name) {
  try {
    ifstream gestures_file;
    string current_line;

    gestures_file.open(file_name.c_str());
    if (!gestures_file.is_open()) {
      cout << "ERROR: Unable to open file: " << file_name << endl;
      return;
    }

    while (getline(gestures_file, current_line)) {
      string name, lager;
      stringstream ss(current_line);
      ss >> name >> lager;

      SendGestureSubscriptionMessage(name, lager);
    }

  } catch (interprocess_exception &ex) {
    std::cerr << ex.what() << endl;
  }
}

/* Code structure taken from http://stackoverflow.com/a/12349823 */
DetectedGestureMessage GetDetectedGestureMessage() {
  DetectedGestureMessage message;

  try {
    ostringstream queue_name_stream;
    queue_name_stream << getpid() << "_detected_gestures";
    message_queue mq(open_or_create, queue_name_stream.str().c_str(),
                     MAX_NUM_MSG, MAX_DETECTED_GESTURE_MSG_SIZE);

    message_queue::size_type received_size;
    unsigned int priority;

    stringstream input_stringstream;
    string serialized_string;
    serialized_string.resize(MAX_DETECTED_GESTURE_MSG_SIZE);
    mq.receive(&serialized_string[0], MAX_DETECTED_GESTURE_MSG_SIZE,
               received_size, priority);
    input_stringstream << serialized_string;

    boost::archive::text_iarchive ia(input_stringstream);
    ia >> message;

    cout << "Received gesture \"" << message.get_gesture_name()
         << "\" on queue \"" << queue_name_stream.str() << "\"" << endl;
  } catch (interprocess_exception &ex) {
    std::cerr << ex.what() << endl;
  }

  return message;
}

void SendDetectedGestureMessage(string gesture_name, pid_t destination_pid) {
  try {
    ostringstream queue_name_stream;
    queue_name_stream << destination_pid << "_detected_gestures";

    cout << "Sending gesture \"" << gesture_name << "\" to queue \""
        << queue_name_stream.str() << "\"" << endl;

    message_queue mq(open_or_create, queue_name_stream.str().c_str(),
                     MAX_NUM_MSG, MAX_DETECTED_GESTURE_MSG_SIZE);

    DetectedGestureMessage message(gesture_name);
    std::stringstream output_stringstream;

    boost::archive::text_oarchive output_archive(output_stringstream);
    output_archive << message;

    std::string serialized_string(output_stringstream.str());
    mq.send(serialized_string.data(), serialized_string.size(), 0);
  } catch (interprocess_exception &ex) {
    std::cerr << ex.what() << std::endl;
  }
}
