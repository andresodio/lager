/*
 * liblager_convert.h
 *
 *  Created on: Mar 2, 2015
 *      Author: andres
 */

#ifndef LIBLAGER_CONVERT_H_
#define LIBLAGER_CONVERT_H_

#include <string>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <boost/thread.hpp>

#include <vrpn_Button.h>                // for vrpn_BUTTONCB, etc
#include <vrpn_Tracker.h>               // for vrpn_TRACKERCB, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_ForceDevice.h"           // for vrpn_ForceDevice_Remote, etc
#include "vrpn_Types.h"                 // for vrpn_float64

using std::string;
using std::stringstream;
using std::chrono::system_clock;
using std::chrono::time_point;

#define DISTANCE_INTERVAL_SQUARED 0.0004f //0.4 * (1 cm)^2

class LagerConverter {
 public:
  static LagerConverter* Instance();

  void SetUseButtons(bool use_buttons) {
    use_buttons_ = use_buttons;

    // If using buttons, will only draw when button is pressed
    draw_gestures_ = !use_buttons;
  }

  ~LagerConverter() {
    processing_thread_.interrupt();
    processing_thread_.join();

    if (tracker_) {
      delete tracker_;
    }

    if (button_) {
      delete button_;
    }
  }

  void Start() {
    InitializeTrackers();
    processing_thread_ = boost::thread(&LagerConverter::ProcessSensorEvents, this);
  }

  string BlockingGetLagerString();

 private:
  LagerConverter() {
  }
  ;
  LagerConverter(LagerConverter const&) {
  }
  ;
  LagerConverter& operator=(LagerConverter const&) {
  }
  ;

  static LagerConverter* instance_;

  std::mutex mutex_;
  std::condition_variable lager_rw_condition_;

  bool lager_read_complete_ = false;
  ;
  bool lager_write_complete_ = false;

  boost::thread processing_thread_;
  bool use_buttons_ = false;

  vrpn_Tracker_Remote* tracker_;
  vrpn_Button_Remote* button_;

  vrpn_TRACKERCB last_report_0_;  // last report for sensor 0
  vrpn_TRACKERCB last_report_1_;  // last report for sensor 1

  bool draw_gestures_ = true;

  std::stringstream lager_string_;

  time_point<system_clock> global_last_movement_time_ = system_clock::now();

  time_point<system_clock> sensor_0_last_movement_time_ = system_clock::now();
  time_point<system_clock> sensor_0_last_grouped_movement_time_ =
      system_clock::now();

  time_point<system_clock> sensor_1_last_movement_time_ = system_clock::now();
  time_point<system_clock> sensor_1_last_grouped_movement_time_ =
      system_clock::now();

  char last_sensor_0_letter_ = '_';
  char last_sensor_1_letter_ = '_';

  void InitializeTrackers();
  void ProcessSensorEvents();

  static void VRPN_CALLBACK HandleTrackerChange(void *user_data,
                                                const vrpn_TRACKERCB tracker);
  static void VRPN_CALLBACK HandleButtonChange(void *user_data,
                                               const vrpn_BUTTONCB button);
  static void VRPN_CALLBACK DummyHandleTrackerChange(
      void *user_data, const vrpn_TRACKERCB tracker);

  vrpn_float64 GetDeltaX(const vrpn_TRACKERCB& last_report,
                         const vrpn_TRACKERCB& tracker);
  vrpn_float64 GetDeltaY(const vrpn_TRACKERCB& last_report,
                         const vrpn_TRACKERCB& tracker);
  vrpn_float64 GetDeltaZ(const vrpn_TRACKERCB& last_report,
                         const vrpn_TRACKERCB& tracker);

  double GetMovementThetaInDegrees(vrpn_float64 delta_x, vrpn_float64 delta_y,
                                   vrpn_float64 delta_z);
  double GetMovementPhiInDegrees(vrpn_float64 delta_x, vrpn_float64 delta_y,
                                 double theta);

  int SnapAngle(double aAngle);
  vrpn_float64 GetDistanceSquared(const vrpn_TRACKERCB& last_report,
                                  const vrpn_TRACKERCB& tracker);
  void PrintSensorCoordinates(const vrpn_TRACKERCB& last_report,
                              const vrpn_TRACKERCB& tracker);

  int GetMillisecondsUntilNow(const time_point<system_clock> &last_time);
  int GetMillisecondsSinceTrackerTime(
      const vrpn_TRACKERCB& tracker, const time_point<system_clock> &last_time);
  void UpdateTimePoint(time_point<system_clock>& time_to_update,
                       time_point<system_clock> new_time);
  time_point<system_clock> GetCurrentMovementTime(
        const vrpn_TRACKERCB& tracker);
  bool GesturePaused();

  void ResetLagerString();

  void CalculateMovementDeltas(const vrpn_TRACKERCB& tracker,
                               const vrpn_TRACKERCB* last_report,
                               vrpn_float64& delta_x, vrpn_float64& delta_y,
                               vrpn_float64& delta_z);
  void CalculateMovementAngles(double& theta, double& phi, int& snap_theta,
                               int& snap_phi, vrpn_float64 delta_x,
                               vrpn_float64 delta_y, vrpn_float64 delta_z);

  void MoveHeadToBeginningOfLetterPair();
  char GetCurrentLetter(int snap_phi, int snap_theta);

  void UpdateLagerString(const vrpn_TRACKERCB& tracker, const int snap_theta,
                         const int snap_phi);
  void UpdateTimers(const vrpn_TRACKERCB& tracker);
};

#endif /* LIBLAGER_CONVERT_H_ */
