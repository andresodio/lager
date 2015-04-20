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

/**
 * Converts the movement of input sensors into LaGeR strings.
 */
class LagerConverter {
 public:
  /**
   * Returns a pointer to an instance of the class.
   * Instantiates the class if needed.
   */
  static LagerConverter* Instance();

  /**
   * Sets the use_buttons_ member variable.
   *
   * If true, sensor movements are only converted to LaGeR while a button is
   * being pressed.
   */
  void SetUseButtons(bool use_buttons) {
    use_buttons_ = use_buttons;

    // If using buttons, will only draw when button is pressed
    draw_gestures_ = !use_buttons;
  }

  /**
   * Destructor for this class.
   * Takes care of joining threads and deleting dynamically allocated variables.
   */
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

  /**
   * Initializes the movement tracking callbacks and starts processing sensor
   * events.
   */
  void Start() {
    InitializeTrackers();
    processing_thread_ = boost::thread(&LagerConverter::ProcessSensorEvents, this);
  }

  /**
   * Returns the LaGeR string that corresponds to the latest sensor gesture.
   * If a gesture is underway, this function blocks until the gesture has
   * finished.
   */
  string BlockingGetLagerString();

 private:
  /**
   * Empty constructor for this class.
   */
  LagerConverter() {
  }
  ;

  /**
   * Empty constructor for this class, with a parameter taking a reference to
   * a class instance.
   */
  LagerConverter(LagerConverter const&) {
  }
  ;

  /**
   * Empty implementation of the operator= for this class.
   */
  LagerConverter& operator=(LagerConverter const&) {
  }
  ;

  /// Pointer to an instance of this class
  static LagerConverter* instance_;

  /// Mutex variable use to protect access to other variables
  std::mutex mutex_;
  /// Condition variable used to synchronize threads reading and writing
  /// the LaGeR string for the current gesture
  std::condition_variable lager_rw_condition_;

  /// Indicates that a thread has finished reading a LaGeR string
  bool lager_read_complete_ = false;
  /// Indicates that a thread has finished writing a LaGeR string
  bool lager_write_complete_ = false;

  /// The thread that is used for processing sensor events
  boost::thread processing_thread_;
  /// Indicates whether sensor buttons are used to determine when to convert
  /// movements to LaGeR.
  bool use_buttons_ = false;

  /// Pointer to the current tracker structure
  vrpn_Tracker_Remote* tracker_;
  /// Pointer to the current button structure
  vrpn_Button_Remote* button_;

  /// Last report for sensor 0
  vrpn_TRACKERCB last_report_0_;
  /// Last report for sensor 1
  vrpn_TRACKERCB last_report_1_;

  /// Whether to convert sensor movements to LaGeR or not
  bool draw_gestures_ = true;

  /// The LaGeR string for the current sensor movements
  std::stringstream lager_string_;

  /// The last time there was any movement
  time_point<system_clock> global_last_movement_time_ = system_clock::now();

  /// The last time sensor 0 moved
  time_point<system_clock> sensor_0_last_movement_time_ = system_clock::now();
  /// The last time the movement of sensor 0 was grouped with sensor 1
  time_point<system_clock> sensor_0_last_grouped_movement_time_ =
      system_clock::now();

  /// The last time sensor 1 moved
  time_point<system_clock> sensor_1_last_movement_time_ = system_clock::now();
  /// The last time the movement of sensor 1 was grouped with sensor 0
  time_point<system_clock> sensor_1_last_grouped_movement_time_ =
      system_clock::now();

  /// The LaGeR symbol corresponding to the most recent sensor 0 movemennt
  char last_sensor_0_letter_ = '_';
  /// The LaGeR symbol corresponding to the most recent sensor 1 movement
  char last_sensor_1_letter_ = '_';

  /**
   * Registers and initializes the VRPN sensor handlers.
   */
  void InitializeTrackers();
  /**
   * Main loop for processing sensor movement and button events.
   */
  void ProcessSensorEvents();

  /**
   * Callback that handles changes to the sensor positions.
   */
  static void VRPN_CALLBACK HandleTrackerChange(void *user_data,
                                                const vrpn_TRACKERCB tracker);
  /**
   * Callback that handles changes to the sensor button states.
   */
  static void VRPN_CALLBACK HandleButtonChange(void *user_data,
                                               const vrpn_BUTTONCB button);
  /**
   * Dummy callback that handles the first few sensor position changes during
   * initialization.
   */
  static void VRPN_CALLBACK DummyHandleTrackerChange(
      void *user_data, const vrpn_TRACKERCB tracker);

  /**
   * Takes the previous and current sensor data and returns the change in X
   * axis position.
   */
  vrpn_float64 GetDeltaX(const vrpn_TRACKERCB& last_report,
                         const vrpn_TRACKERCB& tracker);
  /**
   * Takes the previous and current sensor data and returns the change in Y
   * axis position.
   */
  vrpn_float64 GetDeltaY(const vrpn_TRACKERCB& last_report,
                         const vrpn_TRACKERCB& tracker);
  /**
   * Takes the previous and current sensor data and returns the change in Z
   * axis position.
   */
  vrpn_float64 GetDeltaZ(const vrpn_TRACKERCB& last_report,
                         const vrpn_TRACKERCB& tracker);

  /**
   * Takes the change in X, Y, and Z Cartesian coordinates and returns the
   * equivalent theta change in spherical coordinates.
   */
  double GetMovementThetaInDegrees(vrpn_float64 delta_x, vrpn_float64 delta_y,
                                   vrpn_float64 delta_z);
  /**
   * Takes the change in X, Y, and Z Cartesian coordinates and returns the
   * equivalent phi change in spherical coordinates.
   */
  double GetMovementPhiInDegrees(vrpn_float64 delta_x, vrpn_float64 delta_y,
                                 double theta);

  /**
   * Takes an angle and returns that closest angle in 45 degree increments.
   */
  int SnapAngle(double aAngle);

  /**
   * Takes the previous and current sensor data and returns the sum of the
   * square of each X, Y, and Z distance component in Cartesian coordinates.
   */
  vrpn_float64 GetDistanceSquared(const vrpn_TRACKERCB& last_report,
                                  const vrpn_TRACKERCB& tracker);
  /**
   * Takes the previous and current sensor data and prints it.
   */
  void PrintSensorCoordinates(const vrpn_TRACKERCB& last_report,
                              const vrpn_TRACKERCB& tracker);

  /**
   * Takes a time and returns the numberof milliseconds elapsed until now.
   */
  int GetMillisecondsUntilNow(const time_point<system_clock> &last_time);
  /**
   * Takes sensor data and a time, and returns the number of milliseconds
   * elapsed between the sensor's last movement and that time.
   */
  int GetMillisecondsSinceTrackerTime(
      const vrpn_TRACKERCB& tracker, const time_point<system_clock> &last_time);
  /**
   * Takes two time variables and assigns one to the other.
   */
  void UpdateTimePoint(time_point<system_clock>& time_to_update,
                       time_point<system_clock> new_time);
  /**
   * Takes sensor data and returns its movement time.
   */
  time_point<system_clock> GetCurrentMovementTime(
        const vrpn_TRACKERCB& tracker);

  /**
   * Returns whether the current gesture has paused or not.
   */
  bool GesturePaused();

  /**
   * Clears the global LaGeR gesture stringstream.
   */
  void ResetLagerString();

  /**
   * Takes the current and previous sensor data and calculates the position
   * change in X, Y, and Z Cartesian coordinates.
   */
  void CalculateMovementDeltas(const vrpn_TRACKERCB& tracker,
                               const vrpn_TRACKERCB* last_report,
                               vrpn_float64& delta_x, vrpn_float64& delta_y,
                               vrpn_float64& delta_z);
  /**
   * Takes the current and previous sensor data and calculates the movement
   * angles in theta and phi spherical coordinates.
   */
  void CalculateMovementAngles(double& theta, double& phi, int& snap_theta,
                               int& snap_phi, vrpn_float64 delta_x,
                               vrpn_float64 delta_y, vrpn_float64 delta_z);

  /**
   * Moves the global LaGeR stringstream head back to the beginning of a
   * letter pair.
   */
  void MoveHeadToBeginningOfLetterPair();

  /**
   * Takes theta and phi spherical coordinates and returns the corresponding
   * LaGeR symbol.
   */
  char GetCurrentLetter(int snap_theta, int snap_phi);

  /**
   * Takes the current sensor data and the theta and phi spherical coordinates
   * corresponding to its movement, and update the global LaGeR string with
   * the corresponding symbol.
   */
  void UpdateLagerString(const vrpn_TRACKERCB& tracker, const int snap_theta,
                         const int snap_phi);

  /**
   * Takes the current sensor data structure and updates the global movement
   * timers.
   */
  void UpdateTimers(const vrpn_TRACKERCB& tracker);
};

#endif /* LIBLAGER_CONVERT_H_ */
