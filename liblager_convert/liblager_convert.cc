/*
 * liblager_convert.cc
 *
 *  Created on: Mar 2, 2015
 *      Author: Andrés Odio
 */

#include <string>
#include <time.h>     // for nanosleep

#include "spherical_coordinates.h"
#include "coordinates_letter.h"

#include "liblager_convert.h"

using std::cout;
using std::endl;
using std::string;

using std::chrono::duration;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::chrono::time_point;

#define MAIN_SLEEP_INTERVAL_MICROSECONDS 10000 // 10ms
#define MAIN_SLEEP_INTERVAL_MILLISECONDS MAIN_SLEEP_INTERVAL_MICROSECONDS/1000
#define GESTURE_PAUSE_TIME_MILLISECONDS 500
#define GESTURE_GROUPING_TIME_MILLISECONDS 200
#define SINGLE_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT 20
#define DUAL_SENSOR_GESTURE_DISTANCE_THRESHOLD_PCT 30

#define TRACKER_SERVER "Filter0@localhost" // Filtered
#define BUTTON_SERVER "Tracker0@localhost" // Raw

LagerConverter* LagerConverter::instance_ = NULL;

LagerConverter* LagerConverter::Instance() {
  if (!instance_) {
    instance_ = new LagerConverter;
  }

  return instance_;
}

vrpn_float64 LagerConverter::GetDeltaX(const vrpn_TRACKERCB& last_report,
                                       const vrpn_TRACKERCB& tracker) {
  return (last_report.pos[1] - tracker.pos[1]);
}

vrpn_float64 LagerConverter::GetDeltaY(const vrpn_TRACKERCB& last_report,
                                       const vrpn_TRACKERCB& tracker) {
  return (tracker.pos[0] - last_report.pos[0]);
}

vrpn_float64 LagerConverter::GetDeltaZ(const vrpn_TRACKERCB& last_report,
                                       const vrpn_TRACKERCB& tracker) {
  return (tracker.pos[2] - last_report.pos[2]);
}

double LagerConverter::GetMovementThetaInDegrees(vrpn_float64 delta_x,
                                                 vrpn_float64 delta_y,
                                                 vrpn_float64 delta_z) {
  double movementTheta = acos(
      delta_z
          / (sqrt(pow(delta_x, 2.0) + pow(delta_y, 2.0) + pow(delta_z, 2.0))))
      * (180.0 / M_PI);
  return fmod(movementTheta + 360.0, 360.0);  // Always returns a positive angle
}

double LagerConverter::GetMovementPhiInDegrees(vrpn_float64 delta_x,
                                               vrpn_float64 delta_y,
                                               double theta) {
  int snap_theta = SnapAngle(theta);
  if (snap_theta == 0 || snap_theta == 180) {
    return 0;  // Phi becomes meaningless at the north and south poles, so we default it to 0.
  }

  double movementPhi = atan2(delta_y, delta_x) * (180.0 / M_PI);
  return fmod(movementPhi + 360.0, 360.0);  // Always returns a positive angle
}

/* Snaps angle to 45 degree intervals */
int LagerConverter::SnapAngle(double aAngle) {
  return 45 * (round(aAngle / 45.0));
}

vrpn_float64 LagerConverter::GetDistanceSquared(
    const vrpn_TRACKERCB& last_report, const vrpn_TRACKERCB& tracker) {
  return (last_report.pos[0] - tracker.pos[0])
      * (last_report.pos[0] - tracker.pos[0])
      + (last_report.pos[1] - tracker.pos[1])
          * (last_report.pos[1] - tracker.pos[1])
      + (last_report.pos[2] - tracker.pos[2])
          * (last_report.pos[2] - tracker.pos[2]);
}

#define abs(x) ((x)<0 ? -(x) : (x))

void LagerConverter::PrintSensorCoordinates(const vrpn_TRACKERCB& last_report,
                                            const vrpn_TRACKERCB& tracker) {
  printf("\nSensor %d is now at (%g,%g,%g)\n", tracker.sensor, tracker.pos[0],
         tracker.pos[1], tracker.pos[2]);
  printf("old/new X: %g, %g\n", last_report.pos[1], tracker.pos[1]);
  printf("old/new Y: %g, %g\n", last_report.pos[0], tracker.pos[0]);
  printf("old/new Z: %g, %g\n", last_report.pos[2], tracker.pos[2]);
}

char LagerConverter::GetCurrentLetter(int snap_theta, int snap_phi) {
  struct SphericalCoordinates current_coordinates;
  current_coordinates.theta = snap_theta;
  current_coordinates.phi = snap_phi;
  return coordinates_letter[current_coordinates];
}

int LagerConverter::GetMillisecondsUntilNow(
    const time_point<system_clock> &last_time) {
  time_point < system_clock > now = system_clock::now();
  return duration<double, std::milli>(now - last_time).count();
}

int LagerConverter::GetMillisecondsSinceTrackerTime(
    const vrpn_TRACKERCB& tracker, const time_point<system_clock> &last_time) {
  time_point < system_clock
      > current_movement_time(
          seconds(tracker.msg_time.tv_sec)
              + microseconds(tracker.msg_time.tv_usec));
  return duration<double, std::milli>(current_movement_time - last_time).count();
}

void LagerConverter::UpdateTimePoint(time_point<system_clock>& time_to_update,
                                     time_point<system_clock> new_time) {
  time_to_update = new_time;
}

bool LagerConverter::GesturePaused() {
  return (GetMillisecondsUntilNow(global_last_movement_time_)
      > GESTURE_PAUSE_TIME_MILLISECONDS);
}

void LagerConverter::ResetLagerString() {
  lager_string_.str("");
}

void LagerConverter::CalculateMovementDeltas(const vrpn_TRACKERCB& tracker,
                                             const vrpn_TRACKERCB* last_report,
                                             vrpn_float64& delta_x,
                                             vrpn_float64& delta_y,
                                             vrpn_float64& delta_z) {
  delta_x = GetDeltaX(*last_report, tracker);
  delta_y = GetDeltaY(*last_report, tracker);
  delta_z = GetDeltaZ(*last_report, tracker);
  //printf("X delta: %g, Y delta: %g, Z delta: %g\n", deltaX, deltaY, deltaZ);
}

void LagerConverter::CalculateMovementAngles(double& theta, double& phi,
                                             int& snap_theta, int& snap_phi,
                                             vrpn_float64 delta_x,
                                             vrpn_float64 delta_y,
                                             vrpn_float64 delta_z) {
  theta = GetMovementThetaInDegrees(delta_x, delta_y, delta_z);
  snap_theta = SnapAngle(theta);
  phi = GetMovementPhiInDegrees(delta_x, delta_y, theta);
  snap_phi = SnapAngle(phi);
}

void LagerConverter::MoveHeadToBeginningOfLetterPair() {
  if (lager_string_.str().size() >= 3) {
    lager_string_.seekp(-3, lager_string_.cur);
  }
}

time_point<system_clock> LagerConverter::GetCurrentMovementTime(
    const vrpn_TRACKERCB& tracker) {
  time_point < system_clock
      > current_movement_time(
          seconds(tracker.msg_time.tv_sec)
              + microseconds(tracker.msg_time.tv_usec));
  return current_movement_time;
}

void LagerConverter::UpdateLagerString(const vrpn_TRACKERCB& tracker,
                                       const int snap_theta,
                                       const int snap_phi) {
  int time_since_sensor_0_last_movement = 0;
  int time_since_sensor_1_last_movement = 0;
  char currentLetter = GetCurrentLetter(snap_theta, snap_phi);

  if (tracker.sensor == 0) {
    last_sensor_0_letter_ = currentLetter;
    time_since_sensor_1_last_movement = GetMillisecondsSinceTrackerTime(
        tracker, sensor_1_last_movement_time_);

    /*
     * If sensor 1 reported new movements since the last time it was grouped,
     * and if it moved not too long ago, group its letter with sensor 0's.
     */
    if ((sensor_1_last_grouped_movement_time_ != sensor_1_last_movement_time_)
        && (time_since_sensor_1_last_movement
            < GESTURE_GROUPING_TIME_MILLISECONDS)) {
      //printf("S0: Grouping with previous S1. Current: %c, last: %c\n", currentLetter, last_sensor_1_letter);
      sensor_0_last_grouped_movement_time_ = GetCurrentMovementTime(tracker);
      sensor_1_last_grouped_movement_time_ = sensor_1_last_movement_time_;
      MoveHeadToBeginningOfLetterPair();
      lager_string_ << currentLetter << last_sensor_1_letter_;
    } else {
      //printf("S0 ELSE. GroupingT=MovementT?: %i, tSS1: %i\n", sensor_1_last_grouped_movement_time == sensor_1_last_movement_time, time_since_sensor_1);
      last_sensor_1_letter_ = '_';
      lager_string_ << currentLetter << last_sensor_1_letter_;
    }
  } else {
    last_sensor_1_letter_ = currentLetter;
    time_since_sensor_0_last_movement = GetMillisecondsSinceTrackerTime(
        tracker, sensor_0_last_movement_time_);

    /*
     * If sensor 0 reported new movements since the last time it was grouped,
     * and if it moved not too long ago, group its letter with sensor 1's.
     */
    if ((sensor_0_last_grouped_movement_time_ != sensor_0_last_movement_time_)
        && (time_since_sensor_0_last_movement
            < GESTURE_GROUPING_TIME_MILLISECONDS)) {
      //printf("S1: Grouping with previous S0. Current: %c, last: %c\n", currentLetter, last_sensor_0_letter);
      sensor_0_last_grouped_movement_time_ = sensor_0_last_movement_time_;
      sensor_1_last_grouped_movement_time_ = GetCurrentMovementTime(tracker);
      MoveHeadToBeginningOfLetterPair();
      lager_string_ << last_sensor_0_letter_ << currentLetter;
    } else {
      //printf("S1 ELSE. GroupingT=MovementT?: %i, tSS0: %i\n", sensor_0_last_grouped_movement_time == sensor_0_last_movement_time, time_since_sensor_0);
      last_sensor_0_letter_ = '_';
      lager_string_ << last_sensor_0_letter_ << currentLetter;
    }
  }
  lager_string_ << ".";

  cout << "Gesture: " << lager_string_.str() << endl;
  cout << endl;
}

void LagerConverter::UpdateTimers(const vrpn_TRACKERCB& tracker) {
  time_point < system_clock > current_movement_time = GetCurrentMovementTime(
      tracker);
  UpdateTimePoint(global_last_movement_time_, current_movement_time);

  if (tracker.sensor == 0) {
    UpdateTimePoint(sensor_0_last_movement_time_, current_movement_time);
  } else {
    UpdateTimePoint(sensor_1_last_movement_time_, current_movement_time);
  }

  //printf("Updated. GLMT: %i, S0LMT: %i, S1LMT: %i\n", GetMillisecondsSinceTrackerTime(tracker, global_last_movement_time), GetMillisecondsSinceTrackerTime(tracker, sensor_0_last_movement_time), GetMillisecondsSinceTrackerTime(tracker, sensor_1_last_movement_time));
}

void VRPN_CALLBACK LagerConverter::HandleTrackerChange(
    void *user_data, const vrpn_TRACKERCB tracker) {

  //printf("%s:%i\n", __FUNCTION__, __LINE__);
  LagerConverter* lager_converter = LagerConverter::Instance();
  vrpn_TRACKERCB *last_report;  // keep track of the current sensor's last report
  vrpn_float64 deltaX, deltaY, deltaZ;
  double theta, phi;
  int snap_theta, snap_phi;
  static float dist_interval_sq = DISTANCE_INTERVAL_SQUARED;

  if (!lager_converter->draw_gestures_) {
    return;
  }

  if (tracker.sensor == 0) {
    last_report = &lager_converter->last_report_0_;
  } else {
    last_report = &lager_converter->last_report_1_;
  }

  if (lager_converter->GetDistanceSquared(*last_report, tracker)
      > dist_interval_sq) {
    printf("Update for sensor: %i at time: %ld.%06ld\n", tracker.sensor,
           tracker.msg_time.tv_sec, tracker.msg_time.tv_usec);
    //printf("GLMT: %i, S0LMT: %i, S1LMT: %i\n", GetMillisecondsSinceTrackerTime(tracker, global_last_movement_time), GetMillisecondsSinceTrackerTime(tracker, sensor_0_last_movement_time), GetMillisecondsSinceTrackerTime(tracker, sensor_1_last_movement_time));
    //printSensorCoordinates(last_report, tracker);

    lager_converter->UpdateTimers(tracker);

    lager_converter->CalculateMovementDeltas(tracker, last_report, deltaX,
                                             deltaY, deltaZ);
    lager_converter->CalculateMovementAngles(theta, phi, snap_theta, snap_phi,
                                             deltaX, deltaY, deltaZ);

    lager_converter->UpdateLagerString(tracker, snap_theta, snap_phi);

    *last_report = tracker;
  }
}

void VRPN_CALLBACK LagerConverter::HandleButtonChange(
    void *user_data, const vrpn_BUTTONCB button) {
  LagerConverter* lager_converter = LagerConverter::Instance();

  if (button.state == 1) {
    lager_converter->draw_gestures_ = true;
  } else {
    lager_converter->draw_gestures_ = false;
  }
}

void VRPN_CALLBACK LagerConverter::DummyHandleTrackerChange(
    void *user_data, const vrpn_TRACKERCB tracker) {
  LagerConverter* lager_converter = LagerConverter::Instance();

  if (tracker.sensor == 0) {
    lager_converter->last_report_0_ = tracker;
  } else {
    lager_converter->last_report_1_ = tracker;
  }

  int *num_reports_received = (int *) user_data;
  *num_reports_received = *num_reports_received + 1;
}

void LagerConverter::InitializeTrackers() {
  struct timespec sleep_interval = { 0, MAIN_SLEEP_INTERVAL_MICROSECONDS };
  int num_reports_received = 0;

  // Initialize the tracker_ handler
  tracker_ = new vrpn_Tracker_Remote(TRACKER_SERVER);
  tracker_->register_change_handler(&num_reports_received,
                                    DummyHandleTrackerChange);

  // Skip the first reports, which are spurious (sensor initialization)
  while (num_reports_received != 2) {
    nanosleep(&sleep_interval, NULL);
    tracker_->mainloop();
  }

  // Finish initializing the tracker_
  tracker_->unregister_change_handler(&num_reports_received,
                                      DummyHandleTrackerChange);
  tracker_->register_change_handler(NULL, HandleTrackerChange);

  if (use_buttons_) {
    // Initialize the button_ handler
    button_ = new vrpn_Button_Remote(BUTTON_SERVER);
    button_->register_change_handler(NULL, HandleButtonChange);
  }
}

void LagerConverter::ProcessSensorEvents() {
  while (true) {
    if (use_buttons_) {
      // Let button handler receive button status from the remote button
      button_->mainloop();
    }

    // Let tracker handler receive position information from the remote tracker
    tracker_->mainloop();

    // If gesture buildup pauses, finish converting it
    int lager_string_length = lager_string_.str().length();
    if (GesturePaused() && lager_string_length > 0) {
      /*
       * Only try to convert gestures with more than one movement pair.
       * This reduces spurious conversions from inadvertent movements.
       */
      if (lager_string_length > 4) {
        {
          std::lock_guard < std::mutex > lock(mutex_);
          lager_read_complete_ = false;
          lager_write_complete_ = true;
          lager_rw_condition_.notify_one();
        }

        {
          std::unique_lock < std::mutex > lock(mutex_);
          lager_rw_condition_.wait(lock, [this] {return lager_read_complete_;});
          lager_write_complete_ = false;
        }
      }

      cout << " ________________________________ " << endl;
      cout << "|                                |" << endl;
      cout << "|       COLLECTING DATA...       |" << endl;
      cout << "|________________________________|" << endl;
      cout << "                                  " << endl;

      ResetLagerString();
    }

    // Sleep so we don't take up 100% of CPU
    boost::this_thread::sleep_for(boost::chrono::microseconds{MAIN_SLEEP_INTERVAL_MICROSECONDS});
  }
}

string LagerConverter::BlockingGetLagerString() {
  static string temp_string("");

  std::unique_lock < std::mutex > lock(mutex_);
  lager_rw_condition_.wait(lock, [this] {return lager_write_complete_;});
  lager_write_complete_ = false;

  temp_string = lager_string_.str();

  lager_read_complete_ = true;
  lager_rw_condition_.notify_one();

  return temp_string;
}
